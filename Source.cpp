#include<mpi.h>
#include <iostream>
#include<stdio.h>
#include <math.h>
#include <stdlib.h>
#include<string.h>


#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>


using namespace std;
using namespace msclr::interop;

//Make struct to hold Data of Image
typedef struct
{
	int** pixels;
	int width;
	int height;

}Image;

//used to intialize the 2D array of Image
void allocate_image(Image* u, int Width, int Height)
{
	u->width = Width;
	u->height = Height;

	u->pixels = new int* [Height];
	for (int i = 0; i < Height; i++)
		u->pixels[i] = new int[Width];

}

//used to free the 2D array of Image
void Deallocate_image(Image* u, int Height)
{
	for (int i = 0; i < Height; i++)
		free(u->pixels[i]);

	free(u->pixels);

}

//used to convert 1D array to 2D array
void convert_JPEG_toImage( int* Pixels, Image* u)
{
	for (int i = 0; i < u->width; i++)
	{
		for (int j = 0; j < u->height; j++)
		{
			u->pixels[i][j] = Pixels[(i * u->width) + j];
		}
	}


}

//used to convert 2D array to 1D
void convert_Image_toJPEG(int* Pixels, Image* u)
{
	for (int i = 0; i < u->height; i++)
	{
		for (int j = 0; j < u->width; j++)
		{
			Pixels[(i * u->width) + j] = u->pixels[i][j];
		}
	}

}

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrays*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int *Red = new int[BM.Height * BM.Width];
	int *Green = new int[BM.Height * BM.Width];
	int *Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height*BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i*BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i*width + j] < 0)
			{
				image[i*width + j] = 0;
			}
			if (image[i*width + j] > 255)
			{
				image[i*width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".PNG");
	cout << "result Image Saved " << index << endl;
}


int main()
{   
	int start_s, stop_s, TotalTime = 0;
	start_s = clock();

	MPI_Init(NULL, NULL);

	//get rank of each processor
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	cout << "Rank :" << rank << endl;

	//get size of each processor
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	cout << "Size :" << size << endl;

	//input Image width and height
	int ImageWidth = 500, ImageHeight = 500;

	int NumberOfImage = 4;
	char*ImageSource[14] =
	{  
		"..//Data//Input//1.png",
		"..//Data//Input//testCase(N).png",
		"..//Data//Input//testCase(2N).png",
		"..//Data//Input//testCase(5N).png",
		"..//Data//Input//testCase(10N).png",
		"..//Data//Input//test(1).jpg",
		"..//Data//Input//test(2).jpg",
		"..//Data//Input//test(3).jpg",
		"..//Data//Input//test(4).jpg",
		"..//Data//Input//test(5).jpg",
		"..//Data//Input//test(6).jpg",
		"..//Data//Input//test(7).jpg",
		"..//Data//Input//test(8).jpg",
		"..//Data//Input//test(9).jpg",

	};
	//intialize loop to make high pass filter on different image
	for (int i = 0; i < NumberOfImage; i++)
	{
		int* imageData = NULL;
		System::String^ imagePath;
		std::string img;
		img = ImageSource[i];
		imagePath = marshal_as<System::String^>(img);
		imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);



		int KernelWidth = 3, Kernelheight;
		int kernellength = 9;
		int filter[9] = { 0,-1,0,-1,4,-1,0,-1,0};


		//define Image Information
		int Total_Number_Of_Pixels = ImageWidth * ImageHeight;
		int* Local_Part_Image = new int[Total_Number_Of_Pixels / size];
		int* FilteredImage = new int[Total_Number_Of_Pixels];
		int Block_of_pixels = Total_Number_Of_Pixels / size;
		int counter = 0;
		int start = rank * Block_of_pixels;
		int end = start + Block_of_pixels;



		//Now Every processor have it's pixels to work on it
		if (rank != size - 1)
		{
			for (int i = start; i < end; i++)
			{
				int newPixel = 0;
				int index = i;
				int Movedwidth = 1;
				for (int j = 0; j < kernellength; j++)
				{
					newPixel += imageData[index] * filter[j];


					//if check if the kernel width for fisrt pixel finish get the pixel in 2nd row of original image  
					//to contiune mutiply the kernel
					if (Movedwidth % KernelWidth == 0)
						index += (ImageWidth - 2);
					else
						index++;


					Movedwidth++;

				}

				if (rank == 0)
					Local_Part_Image[i] = newPixel;

				else if (rank > 0)
					Local_Part_Image[counter++] = newPixel;


			}
		}

		if (rank == size - 1)
		{
			end -= (2 * ImageWidth);
			for (int i = start; i < end; i++)
			{
				int newPixel = 0;
				int index = i;
				int Movedwidth = 1;
				for (int j = 0; j < kernellength; j++)
				{
					newPixel += imageData[index] * filter[j];


					//if check if the kernel width for fisrt pixel finish get the pixel in 2nd row of original image  
					//to contiune mutiply the kernel
					if (Movedwidth % KernelWidth == 0)
						index += (ImageWidth - 2);
					else
						index++;


					Movedwidth++;

				}

				if (rank == 0)
					Local_Part_Image[i] = newPixel;

				else if (rank > 0)
					Local_Part_Image[counter++] = newPixel;


			}
		}

		MPI_Gather(Local_Part_Image, Block_of_pixels, MPI_INT, FilteredImage, Block_of_pixels, MPI_INT, 0, MPI_COMM_WORLD);

		if (rank == 0)
		{
			createImage(FilteredImage, ImageWidth, ImageHeight, i);
			stop_s = clock();
			TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
			cout << "time: " << TotalTime << endl;

		}

		delete[]imageData;
		delete[]Local_Part_Image;
		delete[]FilteredImage;

	}
		
	
	

	

	MPI_Finalize();

	
	return 0;

}



