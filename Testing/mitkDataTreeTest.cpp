/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#include <mitkDataTree.h>

#include <mitkImage.h>
#include <mitkLevelWindow.h>
#include <mitkLevelWindowProperty.h>

#include "mitkReferenceCountWatcher.h"

#include "itkTextOutput.h"

#include <fstream>

#ifdef WIN32
#include <crtdbg.h>
#define memcheck                                            \
  if(_CrtCheckMemory()==false)                              \
  {                                                         \
    std::cout<<"Memory check failed. [FAILED]"<<std::endl;  \
    return EXIT_FAILURE;                                    \
  }
#else
#define memcheck
#endif

int mitkDataTreeTest(int /*argc*/, char* /*argv*/[])
{
  itk::OutputWindow::SetInstance(
       itk::TextOutput::New().GetPointer() );

  //Create Image out of nowhere
	mitk::Image::Pointer image;
	mitk::PixelType pt(typeid(int));
	unsigned int dim[]={100,100,20};

  std::cout << "Creating image: ";
	image = mitk::Image::New();
  //image->DebugOn();
	image->Initialize(mitk::PixelType(typeid(int)), 3, dim);
  int *p = (int*)image->GetData();

  int size = dim[0]*dim[1]*dim[2];
  int i;
  for(i=0; i<size; ++i, ++p)
    *p=i;
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of newly created image: ";
  mitk::ReferenceCountWatcher::Pointer imageWatcher = new mitk::ReferenceCountWatcher(image, "image");
  mitk::ReferenceCountWatcher::Pointer nodeWatcher;
  if(imageWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  /*------------------- Part1 -------------------*/
  std::cout << "Testing part1: DataTreeNode without adding to tree"<<std::endl;
  {
    std::cout << "Temporarily create node and reference image: ";
    mitk::DataTreeNode::Pointer node = mitk::DataTreeNode::New();
    node->SetData(image);
    std::cout<<"[PASSED]"<<std::endl;

    nodeWatcher = new mitk::ReferenceCountWatcher(node, "non-added node");

    std::cout << "Testing reference count of image after referencing from node: ";
    if(imageWatcher->GetReferenceCount()!=2)
    {
      std::cout<<"[FAILED]"<<std::endl;
      return EXIT_FAILURE;
    }
    else
      std::cout<<"[PASSED]"<<std::endl;
  }

  std::cout << "Testing reference count of auto-deleted node: ";
  if(nodeWatcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of image after auto-delete of node: ";
  if(imageWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  /*------------------- Part2 -------------------*/
  std::cout << "Testing part2: DataTreeNode with adding to tree"<<std::endl;

  std::cout << "Creating node and reference image: ";
  mitk::DataTreeNode::Pointer node = mitk::DataTreeNode::New();
  nodeWatcher = new mitk::ReferenceCountWatcher(node, "first order child node");
  node->SetData(image);
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Creating tree: ";
  mitk::DataTree::Pointer tree;
  tree=mitk::DataTree::New(); //@FIXME: da DataTreeIteratorClone keinen Smartpointer auf DataTree h�lt, wird tree sonst gel�scht.
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Creating iterator on tree: ";
  mitk::DataTreePreOrderIterator it(tree);
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Adding node via iterator: ";
  it.Add(node);
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of node after adding node to tree: ";
  if(nodeWatcher->GetReferenceCount()!=2)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Creating level-window property: ";
  mitk::LevelWindowProperty::Pointer levWinProp = new mitk::LevelWindowProperty();
  mitk::ReferenceCountWatcher::Pointer propertyWatcher = new mitk::ReferenceCountWatcher(levWinProp, "property");
  mitk::LevelWindow levelwindow;
  levelwindow.SetAuto( image );
  levWinProp->SetLevelWindow( levelwindow );
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of property: ";
  if(propertyWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Adding level-window property: ";
  node->SetProperty( "levelwindow", levWinProp );
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count after removing own reference to added property: ";
  levWinProp = NULL; memcheck;
  if(propertyWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count after removing own reference to added node: ";
  node = NULL; memcheck;
  if(nodeWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Remove node after incrementing iterator: ";
  ++it;
  if (it.Remove()) std::cout<<"[PASSED]"<<std::endl;
  else std::cout<<"[FAILED]"<<std::endl;
  memcheck;

  std::cout << "Testing whether node was really freed: ";
  if(nodeWatcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing whether property was really freed: ";
  if(propertyWatcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing whether reference count of image was really decreased: ";
  if(imageWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  /*------------------- Part3 -------------------*/
  std::cout << "Testing part2: Removing node with subnodes" << std::endl;

  std::cout << "Re-creating node and reference image: ";
  node = mitk::DataTreeNode::New();
  nodeWatcher = new mitk::ReferenceCountWatcher(node, "first order child node in second order tree");
  node->SetData(image);
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing iterator::GoToBegin(): ";
  it.GoToBegin();
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Adding node via iterator: ";
  it.Add(node);
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Go to first first child: iterator::GoToChild(): ";
  it.GoToChild();
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Create another node and reference another image: ";
  mitk::DataTreeNode::Pointer node2 = mitk::DataTreeNode::New();
  mitk::ReferenceCountWatcher::Pointer node2Watcher = new mitk::ReferenceCountWatcher(node2, "second order child node");
  mitk::Image::Pointer image2 = mitk::Image::New();
  mitk::ReferenceCountWatcher::Pointer image2Watcher = new mitk::ReferenceCountWatcher(image2, "image2");
  node2->SetData(image2);
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Adding node2 via iterator (sub-node): ";
  it.Add(node2);
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of node after adding node2 to the tree: ";
  if(nodeWatcher->GetReferenceCount()!=2)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Removing own references to added nodes and images: ";
  node = NULL; memcheck; node2 = NULL; memcheck;
  image = NULL; memcheck; image2 = NULL; memcheck;
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of node after removing own reference: ";
  if(nodeWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of node2 after removing own reference: ";
  if(node2Watcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of image after removing own reference: ";
  if(imageWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing reference count of image2 after removing own reference: ";
  if(image2Watcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Do iterator::GoToBegin() and iterator::GoToChild(): ";
  it.GoToBegin();
  it.GoToChild();
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Remove node and its child: ";
  if (it.Remove()) std::cout<<"[PASSED]"<<std::endl;
  else std::cout<<"[FAILED]"<<std::endl;
  memcheck;

  std::cout << "Testing whether node was really freed: ";
  if(nodeWatcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing whether node2 was really freed: ";
  if(node2Watcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing whether image was really freed: ";
  if(imageWatcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Testing whether image2 was really freed: ";
  if(image2Watcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;


  // some iterator tests
  // start with new datatree
  tree = mitk::DataTree::New(); //@FIXME: da DataTreeIteratorClone keinen Smartpointer auf DataTree h�lt, wird tree sonst gel�scht.
  std::cout << "Now doing an iterator test. Create another node with another image: " << std::flush;
  mitk::DataTreeNode::Pointer node3 = mitk::DataTreeNode::New();
  mitk::ReferenceCountWatcher::Pointer node3Watcher = new mitk::ReferenceCountWatcher(node3, "node3");
  mitk::Image::Pointer image3 = mitk::Image::New();
  mitk::ReferenceCountWatcher::Pointer image3Watcher = new mitk::ReferenceCountWatcher(image3, "image3");
  node3->SetData(image3);
  image3 = NULL;
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Adding node3 via iterator by root-iterator: " << std::flush;
  mitk::DataTreePreOrderIterator rootIt(tree);
  rootIt.Add(node3);
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Clone root-iterator: " << std::flush;
  mitk::DataTreePreOrderIterator* res = static_cast<mitk::DataTreePreOrderIterator*>(rootIt.Clone());
  if(res == NULL)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Find node3 again and go there: " << std::flush;
  res->GoToChild(res->ChildPosition(node3));
  if((res->IsAtEnd()) || (res->Get() != node3.GetPointer()))
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Remove node3 from tree via iterator: " << std::flush;
  res->Remove();
  std::cout<<"[PASSED]"<<std::endl;
  
  node3 = NULL;
  std::cout << "Testing whether image3 was really freed: ";
  if(image3Watcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  // testing destruction of complete tree
  // start with new datatree
  tree = mitk::DataTree::New(); //@FIXME: da DataTreeIteratorClone keinen Smartpointer auf DataTree h�lt, wird tree sonst gel�scht.
  std::cout << "Now preparing to test desctruction of complete tree: " << std::endl;
  mitk::DataTreePreOrderIterator rootIt2(tree);
  mitk::DataTreeNode::Pointer nodeEmpty = mitk::DataTreeNode::New();
  mitk::ReferenceCountWatcher::Pointer nodeEmptyWatcher = new mitk::ReferenceCountWatcher(nodeEmpty, "nodeEmpty");
  rootIt2.Add(nodeEmpty);
  nodeEmpty = NULL;
  mitk::DataTreeNode::Pointer node4 = mitk::DataTreeNode::New();
  mitk::ReferenceCountWatcher::Pointer node4Watcher = new mitk::ReferenceCountWatcher(node4, "node4");
  mitk::Image::Pointer image4 = mitk::Image::New();
  mitk::ReferenceCountWatcher::Pointer image4Watcher = new mitk::ReferenceCountWatcher(image4, "image4");
  node4->SetData(image4);
  image4 = NULL;
  rootIt2.Add(node4);
  std::cout << "Find node4 again and go there: " << std::flush;
  res = static_cast<mitk::DataTreePreOrderIterator*>(rootIt2.Clone());
  res->GoToChild(res->ChildPosition(node4));
  if((res->IsAtEnd()) || (res->Get() != node4.GetPointer()))
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  node4 = NULL;
  mitk::DataTreeNode::Pointer node5 = mitk::DataTreeNode::New();
  mitk::ReferenceCountWatcher::Pointer node5Watcher = new mitk::ReferenceCountWatcher(node5, "node5");
  mitk::Image::Pointer image5 = mitk::Image::New();
  mitk::ReferenceCountWatcher::Pointer image5Watcher = new mitk::ReferenceCountWatcher(image5, "image5");
  node5->SetData(image5);
  image5 = NULL;
  res->Add(node5);
  node5 = NULL;

  mitk::ReferenceCountWatcher::Pointer treeWatcher = new mitk::ReferenceCountWatcher(tree, "tree");
  std::cout << "Before tree destruction: Testing correctness of tree reference count: ";
  if(treeWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "Before tree destruction: Testing correctness of nodeEmpty reference count: ";
  if(nodeEmptyWatcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "Before tree destruction: Testing correctness of node4 reference count: ";
  if(node4Watcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "Before tree destruction: Testing correctness of image4 reference count: ";
  if(image4Watcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "Before tree destruction: Testing correctness of node5 reference count: ";
  if(node5Watcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "Before tree destruction: Testing correctness of image5 reference count: ";
  if(image5Watcher->GetReferenceCount()!=1)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;

  std::cout << "Destroying tree";
  tree = NULL;
  std::cout<<"[PASSED]"<<std::endl;

  std::cout << "After tree destruction: Testing correctness of tree reference count: ";
  if(treeWatcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "After tree destruction: Testing correctness of nodeEmpty reference count: ";
  if(nodeEmptyWatcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "After tree destruction: Testing correctness of node4 reference count: ";
  if(node4Watcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "After tree destruction: Testing correctness of image4 reference count: ";
  if(image4Watcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "After tree destruction: Testing correctness of node5 reference count: ";
  if(node5Watcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;
  std::cout << "After tree destruction: Testing correctness of image5 reference count: ";
  if(image5Watcher->GetReferenceCount()!=0)
  {
    std::cout<<"[FAILED]"<<std::endl;
    return EXIT_FAILURE;
  }
  else
    std::cout<<"[PASSED]"<<std::endl;


  std::cout<<"[TEST DONE]"<<std::endl;
  return EXIT_SUCCESS;
}
