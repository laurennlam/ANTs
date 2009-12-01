/*=========================================================================
  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: AverageImages.cxx,v $
  Language:  C++
  Date:      $Date: 2009/01/27 23:25:24 $
  Version:   $Revision: 1.21 $

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

// We divide the 2nd input image by its mean and add it to the first
// input image with weight 1/n.
// The output overwrites the 1st img with the sum.

// Note: could easily add variance computation
// http://people.revoledu.com/kardi/tutorial/RecursiveStatistic/Time-Variance.htm

#include "ReadWriteImage.h"
#include "itkOptimalSharpeningImageFilter.h"

template <unsigned int ImageDimension>
int AverageImages(unsigned int argc, char *argv[])
{
  typedef float                                                           PixelType;
  typedef itk::Vector<float, ImageDimension>                              VectorType;
  typedef itk::Image<VectorType, ImageDimension>                          FieldType;
  typedef itk::Image<PixelType, ImageDimension>                           ImageType;
  typedef itk::ImageFileReader<ImageType>                                 readertype;
  typedef itk::ImageFileWriter<ImageType>                                 writertype;
  typedef typename ImageType::IndexType                                   IndexType;
  typedef typename ImageType::SizeType                                    SizeType;
  typedef typename ImageType::SpacingType                                 SpacingType;
  typedef itk::AffineTransform<double, ImageDimension>                    AffineTransformType;
  typedef itk::LinearInterpolateImageFunction<ImageType, double>          InterpolatorType1;
  typedef itk::NearestNeighborInterpolateImageFunction<ImageType, double> InterpolatorType2;
  typedef itk::ImageRegionIteratorWithIndex<ImageType>                    Iterator;

  bool  normalizei = atoi(argv[3]);
  float numberofimages = (float)argc - 4.;

  std::cout << " Averaging " << numberofimages << " images " << std::endl;

  typename ImageType::Pointer averageimage = NULL;
  typename ImageType::Pointer image2 = NULL;

  typename ImageType::SizeType size;
  double meanval = 1;
  size.Fill(0);
  unsigned int bigimage = 0;
  for( unsigned int j = 4; j < argc; j++ )
    {
    // Get the image dimension
    std::string fn = std::string(argv[j]);
    std::cout << " fn " << fn << std::endl;
    typename itk::ImageIOBase::Pointer imageIO =
      itk::ImageIOFactory::CreateImageIO(fn.c_str(), itk::ImageIOFactory::ReadMode);
    imageIO->SetFileName(fn.c_str() );
    imageIO->ReadImageInformation();
    for( unsigned int i = 0; i < imageIO->GetNumberOfDimensions(); i++ )
      {
      if( imageIO->GetDimensions(i) > size[i] )
        {
        size[i] = imageIO->GetDimensions(i);
        bigimage = j;
        std::cout << " bigimage " << j << " size " << size << std::endl;
        }
      }
    }

  std::cout << " largest image " << size << std::endl;

  ReadImage<ImageType>(averageimage, argv[bigimage]);
  averageimage->FillBuffer(0);
  for( unsigned int j = 4; j < argc; j++ )
    {
    // Get the image dimension
    std::string fn = std::string(argv[j]);
    //      std::cout <<" fn " << fn << std::endl;
    ReadImage<ImageType>(image2, fn.c_str() );

    unsigned long ct = 0;
    if( normalizei )
      {
      meanval = 0.0;
      Iterator vfIter2( image2,  image2->GetLargestPossibleRegion() );
      for(  vfIter2.GoToBegin(); !vfIter2.IsAtEnd(); ++vfIter2 )
        {
        meanval += vfIter2.Get();
        ct++;
        }
      if( ct > 0 )
        {
        meanval /= (float)ct;
        }
      if( meanval <= 0 )
        {
        meanval = 1.0;
        }
      }

    ct = 0;
    Iterator vfIter( image2,  image2->GetLargestPossibleRegion() );
    for(  vfIter.GoToBegin(); !vfIter.IsAtEnd(); ++vfIter )
      {
      double val =  vfIter.Get() / meanval * 1.0 / numberofimages;
      averageimage->SetPixel(vfIter.GetIndex(),   val + averageimage->GetPixel(vfIter.GetIndex() ) );
      }
    }

  typedef itk::OptimalSharpeningImageFilter<
      ImageType,
      ImageType>    sharpeningFilter;
  typename sharpeningFilter::Pointer shFilter = sharpeningFilter::New();

  if( normalizei && argc > 3 )
    {
    shFilter->SetInput( averageimage );
    shFilter->SetSValue(0.5);
    averageimage =  shFilter->GetOutput();
    }

  std::cout << " writing output ";
    {
    typename writertype::Pointer writer = writertype::New();
    writer->SetFileName(argv[2]);
    writer->SetInput( averageimage );
    writer->Update();
    }

  return 0;
}

int main(int argc, char *argv[])
{
  if( argc < 3 )
    {
    std::cout << "Basic useage ex: " << std::endl;
    std::cout << argv[0] << " ImageDimension  average.nii normalizei  list-of-files-via-wildcard " << std::endl;
    std::cout << " e.g. \n   AverageImages 3  average.nii  1  *registered.nii " << std::endl;
    std::cout << " normalizei=true divides the 2nd image by its mean " << std::endl;
    std::cout << " this image will select the largest image to average into " << std::endl;
    return 1;
    }

  // Get the image dimension
  switch( atoi(argv[1]) )
    {
    case 2:
      {
      AverageImages<2>(argc, argv);
      }
      break;
    case 3:
      {
      AverageImages<3>(argc, argv);
      }
      break;
    default:
      std::cerr << "Unsupported dimension" << std::endl;
      exit( EXIT_FAILURE );
    }

  return 0;
}
