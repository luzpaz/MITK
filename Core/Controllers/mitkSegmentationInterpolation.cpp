#include "mitkSegmentationInterpolation.h"

#include "mitkImageCast.h"
#include "mitkExtractImageFilter.h"

#include "ipSegmentation.h"

#include <itkCommand.h>
#include <itkImage.h>
#include <itkImageSliceConstIteratorWithIndex.h>

mitk::SegmentationInterpolation::InterpolatorMapType mitk::SegmentationInterpolation::s_InterpolatorForImage; // static member initialization
    
mitk::SegmentationInterpolation* mitk::SegmentationInterpolation::InterpolatorForImage(const Image* image)
{
  InterpolatorMapType::iterator iter = s_InterpolatorForImage.find( image );
  if ( iter != s_InterpolatorForImage.end() )
  {
    return iter->second;
  }
  else
  {
    return NULL;
  }
}

mitk::SegmentationInterpolation::SegmentationInterpolation()
:m_BlockModified(false)
{
}

mitk::SegmentationInterpolation::~SegmentationInterpolation()
{
  // remove this from the list of interpolators
  for ( InterpolatorMapType::iterator iter = s_InterpolatorForImage.begin();
        iter != s_InterpolatorForImage.end();
        ++iter )
  {
    if (iter->second == this)
    {
      s_InterpolatorForImage.erase( iter );
      break;
    }
  }
}
    
void mitk::SegmentationInterpolation::OnImageModified(const itk::EventObject&)
{
  if (!m_BlockModified  && m_Segmentation)
  {
    SetSegmentationVolume( m_Segmentation );
  }
}
    
void mitk::SegmentationInterpolation::BlockModified(bool block)
{
  m_BlockModified = block;
}

void mitk::SegmentationInterpolation::SetSegmentationVolume( const Image* segmentation )
{
  // clear old information
  for (unsigned int dim = 0; dim < 3; ++dim)
  {
    m_SegmentationCountInSlice[dim].clear();
  }

  // delete this from the list of interpolators
  InterpolatorMapType::iterator iter = s_InterpolatorForImage.find( segmentation );
  if ( iter != s_InterpolatorForImage.end() )
  {
    s_InterpolatorForImage.erase( iter );
  }

  if (!segmentation) return;
  if (!(segmentation->GetDimension() == 3)) 
  {
    itkExceptionMacro("SegmentationInterpolation needs a 3D-segmentation, not 2D or 3D+t or something else.");
  }

  if (m_Segmentation != segmentation)
  {
    // observe Modified() event of image
    itk::ReceptorMemberCommand<SegmentationInterpolation>::Pointer command = itk::ReceptorMemberCommand<SegmentationInterpolation>::New();
    command->SetCallbackFunction( this, &SegmentationInterpolation::OnImageModified );
    segmentation->AddObserver( itk::ModifiedEvent(), command );
  }

  m_Segmentation = segmentation;

  for (unsigned int dim = 0; dim < 3; ++dim)
  {
    m_SegmentationCountInSlice[dim].clear();
    m_SegmentationCountInSlice[dim].resize( m_Segmentation->GetDimension(dim) );
    m_SegmentationCountInSlice[dim].assign( m_Segmentation->GetDimension(dim), 0 );
  }

  s_InterpolatorForImage.insert( std::make_pair( m_Segmentation, this ) );

  // scan whole image
  AccessFixedDimensionByItk_1( m_Segmentation, ScanWholeVolume, 3, m_Segmentation );

  //PrintStatus();
  
  Modified();
}

void mitk::SegmentationInterpolation::SetChangedVolume( const Image* sliceDiff )
{
  if ( !sliceDiff ) return;
  if ( sliceDiff->GetDimension() != 3 ) return;
  
  AccessFixedDimensionByItk( sliceDiff, ScanChangedVolume, 3 );

  //PrintStatus();
  Modified();
}


void mitk::SegmentationInterpolation::SetChangedSlice( const Image* sliceDiff, unsigned int sliceDimension, unsigned int sliceIndex )
{
  if ( !sliceDiff ) return;
  if ( sliceDimension > 2 ) return;
  if ( sliceIndex >= m_SegmentationCountInSlice[sliceDimension].size() ) return;

  unsigned int dim0(0);
  unsigned int dim1(1);

  // determine the other two dimensions
  switch (sliceDimension)
  {
    default:
    case 2:
      dim0 = 0; dim1 = 1; break;
    case 1:
      dim0 = 0; dim1 = 2; break;
    case 0:
      dim0 = 1; dim1 = 2; break;
  }

  ipPicDescriptor* rawSlice = const_cast<Image*>(sliceDiff)->GetSliceData()->GetPicDescriptor(); // we promise not to change anything!
  if (!rawSlice) return;

  AccessFixedDimensionByItk_1( sliceDiff, ScanChangedSlice, 2, SetChangedSliceOptions(sliceDimension, sliceIndex, dim0, dim1, rawSlice->data) );
  
  //PrintStatus();
  
  Modified();
}

template < typename DATATYPE >
void mitk::SegmentationInterpolation::ScanChangedSlice( itk::Image<DATATYPE, 2>*, const SetChangedSliceOptions& options )
{
  DATATYPE* pixelData( (DATATYPE*)options.pixelData );

  unsigned int sliceDimension( options.sliceDimension );
  unsigned int sliceIndex( options.sliceIndex );

  if ( sliceDimension > 2 ) return;
  if ( sliceIndex >= m_SegmentationCountInSlice[sliceDimension].size() ) return;
  
  unsigned int dim0( options.dim0 );
  unsigned int dim1( options.dim1 );

  int numberOfPixels(0); // number of pixels in this slice that are not 0

  unsigned int dim0max = m_SegmentationCountInSlice[dim0].size();
  unsigned int dim1max = m_SegmentationCountInSlice[dim1].size();

  // scan the slice from two directions
  // and set the flags for the two dimensions of the slice
  for (unsigned int v = 0; v < dim1max; ++v)
  {
    for (unsigned int u = 0; u < dim0max; ++u)
    {
      DATATYPE value = *(pixelData + u + v * dim0max);

      assert ( m_SegmentationCountInSlice[dim0][u] + value >= 0 ); // just for debugging. This must always be true, otherwise some counting is going wrong
      assert ( m_SegmentationCountInSlice[dim1][v] + value >= 0 );

      m_SegmentationCountInSlice[dim0][u] = static_cast<unsigned int>( m_SegmentationCountInSlice[dim0][u] + value ); 
      m_SegmentationCountInSlice[dim1][v] = static_cast<unsigned int>( m_SegmentationCountInSlice[dim1][v] + value );
      numberOfPixels += static_cast<int>( value );
    }
  }

  // flag for the dimension of the slice itself 
  assert ( m_SegmentationCountInSlice[sliceDimension][sliceIndex] + numberOfPixels >= 0 );
  m_SegmentationCountInSlice[sliceDimension][sliceIndex] += numberOfPixels;
  
  //std::cout << "scan from (0,0) to (" << dim0max << "," << dim1max << ") (" << pixelData << "-" << pixelData+dim0max*dim1max-1 <<  ") in slice " << sliceIndex << " found " << numberOfPixels << " pixels" << std::endl;
}


template < typename TPixel, unsigned int VImageDimension >
void mitk::SegmentationInterpolation::ScanChangedVolume( itk::Image<TPixel, VImageDimension>* diffImage )
{
  typedef itk::ImageSliceConstIteratorWithIndex< itk::Image<TPixel, VImageDimension> > IteratorType;

  IteratorType iter( diffImage, diffImage->GetLargestPossibleRegion() );
  iter.SetFirstDirection(0);
  iter.SetSecondDirection(1);

  int numberOfPixels(0); // number of pixels in this slice that are not 0
  
  typename IteratorType::IndexType index;
  unsigned int x;
  unsigned int y;
  unsigned int z;

  iter.GoToBegin();
  while ( !iter.IsAtEnd() )
  {
    while ( !iter.IsAtEndOfSlice() )
    {
      while ( !iter.IsAtEndOfLine() )
      {
        index = iter.GetIndex();

        x = index[0];
        y = index[1];
        z = index[2];

        TPixel value = iter.Get();

        assert ( m_SegmentationCountInSlice[0][x] + value >= 0 ); // just for debugging. This must always be true, otherwise some counting is going wrong
        assert ( m_SegmentationCountInSlice[1][y] + value >= 0 );

        m_SegmentationCountInSlice[0][x] = static_cast<unsigned int>( m_SegmentationCountInSlice[0][x] + value ); 
        m_SegmentationCountInSlice[1][y] = static_cast<unsigned int>( m_SegmentationCountInSlice[1][y] + value );
  
        numberOfPixels += static_cast<int>( value );
        
        ++iter;
      }
      iter.NextLine();
    }
    assert ( m_SegmentationCountInSlice[2][z] + numberOfPixels >= 0 );
    m_SegmentationCountInSlice[2][z] += numberOfPixels;
    numberOfPixels = 0;

    iter.NextSlice();
  }
}

    
template < typename DATATYPE >
void mitk::SegmentationInterpolation::ScanWholeVolume( itk::Image<DATATYPE, 3>*, const Image* volume )
{
  if (!volume) return;

  for (unsigned int slice = 0; slice < volume->GetDimension(2); ++slice)
  {
    DATATYPE* rawVolume = static_cast<DATATYPE*>( const_cast<Image*>(volume)->GetData() ); // we again promise not to change anything, we'll just count
    //DATATYPE* rawSlice = static_cast<DATATYPE*>( volume->GetSliceData(slice)->GetData() ); // TODO THIS wouldn't work. Did I mess up with some internal mitk::Image data structure?
    DATATYPE* rawSlice = rawVolume + ( volume->GetDimension(0) * volume->GetDimension(1) * slice );

    ScanChangedSlice<DATATYPE>( NULL, SetChangedSliceOptions(2, slice, 0, 1, rawSlice) );
  }
}

void mitk::SegmentationInterpolation::PrintStatus()
{
  std::cout << "Interpolator status: dimensions " 
            << m_SegmentationCountInSlice[0].size() << " " 
            << m_SegmentationCountInSlice[1].size() << " "
            << m_SegmentationCountInSlice[2].size() << std::endl;
    
  std::cout << "Slice 0: " <<  m_SegmentationCountInSlice[2][0] << std::endl;

  // row "x"
  for (unsigned int index = 0; index < m_SegmentationCountInSlice[0].size(); ++index)
  {
    if ( m_SegmentationCountInSlice[0][index] > 0 )
      std::cout << "O";
    else
      std::cout << ".";
  }
  std::cout << std::endl;
 
  // rows "y" and "z" (diagonal)
  for (unsigned int index = 1; index < m_SegmentationCountInSlice[1].size(); ++index)
  {
    if ( m_SegmentationCountInSlice[1][index] > 0 )
      std::cout << "O";
    else
      std::cout << ".";

    if ( m_SegmentationCountInSlice[2].size() > index ) // if we also have a z value here, then print it, too
    {
      for (unsigned int indent = 1; indent < index; ++indent)
        std::cout << " ";

      if ( m_SegmentationCountInSlice[2][index] > 0 )
        std::cout << m_SegmentationCountInSlice[2][index];//"O";
      else
        std::cout << ".";
    }

    std::cout << std::endl;
  }

  // z indices that are larger than the biggest y index
  for (unsigned int index = m_SegmentationCountInSlice[1].size(); index < m_SegmentationCountInSlice[2].size(); ++index)
  {
    for (unsigned int indent = 0; indent < index; ++indent)
    std::cout << " ";

    if ( m_SegmentationCountInSlice[2][index] > 0 )
      std::cout << m_SegmentationCountInSlice[2][index];//"O";
    else
      std::cout << ".";
    
    std::cout << std::endl;
  }
}

mitk::Image::Pointer mitk::SegmentationInterpolation::Interpolate( unsigned int sliceDimension, unsigned int sliceIndex )
{
  if (m_Segmentation.IsNull()) return NULL;

  if ( sliceDimension > 2 ) return NULL;
  unsigned int upperLimit = m_SegmentationCountInSlice[sliceDimension].size();
  if ( sliceIndex >= upperLimit - 1 ) return NULL; // can't interpolate first and last slice
  if ( sliceIndex < 1  ) return NULL;

  if ( m_SegmentationCountInSlice[sliceDimension][sliceIndex] > 0 ) return NULL; // slice contains a segmentation, won't interpolate anything then

  unsigned int lowerBound(0);
  unsigned int upperBound(0);
  bool bounds( false );

  for (lowerBound = sliceIndex - 1; /*lowerBound >= 0*/; --lowerBound)
  {
    if ( m_SegmentationCountInSlice[sliceDimension][lowerBound] > 0 )
    {
      bounds = true;
      break;
    }

    if (lowerBound == 0) break; // otherwise overflow and start at something like 4294967295
  }

  if (!bounds) return NULL;

  bounds = false;
  for (upperBound = sliceIndex + 1 ; upperBound < upperLimit; ++upperBound)
  {
    if ( m_SegmentationCountInSlice[sliceDimension][upperBound] > 0 )
    {
      bounds = true;
      break;
    }
  }

  if (!bounds) return NULL;
 
  // ok, we have found two neighboring slices with segmentations (and we made sure that the current slice does NOT contain anything
  //std::cout << "Interpolate in dimension " << sliceDimension << ": estimate slice " << sliceIndex << " from slices " << lowerBound << " and " << upperBound << std::endl;

  mitk::Image::Pointer lowerMITKSlice;
  mitk::Image::Pointer upperMITKSlice;
  mitk::Image::Pointer resultImage;
  try
  {
  // extract the two neighoring slices from the segmentation volume
    ExtractImageFilter::Pointer extractor= ExtractImageFilter::New();
    extractor->SetInput( m_Segmentation );
    extractor->SetSliceDimension( sliceDimension );
    extractor->SetSliceIndex( lowerBound );
    extractor->Update();
    lowerMITKSlice = extractor->GetOutput();
    lowerMITKSlice->DisconnectPipeline(); // otherwise the next output of the filter will overwrite this pointer, too
    
    extractor->SetSliceIndex( sliceIndex );
    extractor->Update();
    resultImage = extractor->GetOutput();
    resultImage->DisconnectPipeline();
    
    extractor->SetSliceIndex( upperBound );
    extractor->Update();
    upperMITKSlice = extractor->GetOutput();

    if ( lowerMITKSlice.IsNull() || upperMITKSlice.IsNull() ) return NULL;
  }
  catch(...)
  {
    return NULL;
  }

  // TODO review: copies only when neccessary?
  // convert these slices to the ipSegmentation data type (into an ITK image)
  itk::Image< ipMITKSegmentationTYPE, 2 >::Pointer correctPixelTypeLowerITKSlice;
  CastToItkImage( lowerMITKSlice, correctPixelTypeLowerITKSlice );
  assert ( correctPixelTypeLowerITKSlice.IsNotNull() );

  itk::Image< ipMITKSegmentationTYPE, 2 >::Pointer correctPixelTypeUpperITKSlice;
  CastToItkImage( upperMITKSlice, correctPixelTypeUpperITKSlice );
  assert ( correctPixelTypeUpperITKSlice.IsNotNull() );

  // correct direction info
  itk::Image< ipMITKSegmentationTYPE, 2 >::DirectionType imageDirection;
  imageDirection.SetIdentity();
  correctPixelTypeLowerITKSlice->SetDirection(imageDirection);
  correctPixelTypeUpperITKSlice->SetDirection(imageDirection);

  // back-convert to MITK images to access a ipPicDescriptor
  Image::Pointer correctPixelTypeLowerMITKSlice = Image::New();
  CastToMitkImage( correctPixelTypeLowerITKSlice, correctPixelTypeLowerMITKSlice );
  ipPicDescriptor* lowerSlice = correctPixelTypeLowerMITKSlice->GetSliceData()->GetPicDescriptor();
  
  Image::Pointer correctPixelTypeUpperMITKSlice = Image::New();
  CastToMitkImage( correctPixelTypeUpperITKSlice, correctPixelTypeUpperMITKSlice );
  ipPicDescriptor* upperSlice = correctPixelTypeUpperMITKSlice->GetSliceData()->GetPicDescriptor();

  // calculate where the current slice is in comparison to the lower and upper neighboring slices
  float ratio = (float)(sliceIndex - lowerBound) / (float)(upperBound - lowerBound);

  ipPicDescriptor* ipPicResult = ipMITKSegmentationInterpolate( lowerSlice, upperSlice, ratio ); // magic
  if (!ipPicResult) return NULL;

  Geometry3D::Pointer originalGeometry = resultImage->GetGeometry();
  resultImage->Initialize( ipPicResult );
  resultImage->SetPicSlice( ipPicResult );
  resultImage->SetGeometry( originalGeometry );

  ipPicFree( ipPicResult );

  return resultImage;
}

