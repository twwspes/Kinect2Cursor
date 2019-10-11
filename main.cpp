// Define only one of the following
#define DEPTHCAMERA
// The definition below works only when SKELETONTRACKING && SKELETONTRACKINGGREYSCALE || DEPTHCAMERA is defined
// Define only one of the following
//#define PUTBACKGROUNDDEPTH
#define RETRIEVEBACKGROUNDDEPTH
// Define it if you want to reduce noises by morphology
#define MORPHOLOGY
// Define it if you want limited view range
#define LIMITVIEWRANGE
// Define it if you want Waves.mp4 playing on the BG
//#define WAVEBG
// Define it if you need waterFlow effect, But RETRIEVEBACKGROUNDDEPTH need to be defined.
//#define WATERFLOW
// Define if if you need kinect to work as cursor, DEPTHCAMERA only
//#define CURSOR
#define COLORCAMERA

#include "main.h"

#include "opencv2/opencv.hpp"

//the following three are for file streaming
#include <fstream>
#include <iostream>
#include <string>
#include <thread> 


#include <cmath>
#include <cstdio>

#include <Windows.h>
#include <Ole2.h>

#include <gl/glew.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glut.h>

#include <Kinect.h>

//Using std C++
using namespace std;
//For OpenCV
using namespace cv;
bool breakLoops = false;
//static VideoCapture cap("Waves.mp4");
Mat BGframe;
Mat BGframeCopy;
Vec3b intensity;

#if defined(DEPTHCAMERA) && !defined(LIMITVIEWRANGE)
#define width 512
#define height 424
#endif

#if !defined(LIMITVIEWRANGE)
// OpenGL Variables
GLubyte glBytedata[width * height * 4];  // BGRA array containing the texture data
#ifdef WATERFLOW
unsigned short waterDropArray[width * height] = { 0 };
short currentObjectHeightCache[width * height];
bool _shouldCleanWaterDrop = false;
#endif
#else
#define width 512
#define height 424
#define limitedOffsetX 95
#define limitedOffsetY 104
#define limitedWidth 296
#define limitedHeight 165
#define windowsWidth limitedWidth
#define windowsHeight limitedHeight
GLubyte glBytedata[limitedWidth * limitedHeight * 4];
#ifdef WATERFLOW
unsigned short waterDropArray[limitedWidth * limitedHeight] = { 0 };
short currentObjectHeightCache[limitedWidth * limitedHeight];
bool _shouldCleanWaterDrop = false;
#endif
#endif

bool fullscreen = true;

#ifdef MORPHOLOGY
Mat destCastInMatsrc, destCastInMatdst;

#ifdef LIMITVIEWRANGE

#else

#endif
#endif

#ifdef CURSOR
#include <chrono>
#include <ctime> 
chrono::system_clock::time_point programStartUpTime = chrono::system_clock::now();
bool _shouldCleanCursorMark = false;
#ifdef LIMITVIEWRANGE
short currentObjectHeightCacheForCursor[limitedWidth * limitedHeight];
unsigned short blackColouredArray[limitedWidth * limitedHeight] = { 0 };
chrono::system_clock::time_point potentialCursorPoint[limitedWidth * limitedHeight];
#else
short currentObjectHeightCacheForCursor[width * height];
unsigned short blackColouredArray[width * height] = { 0 };
chrono::system_clock::time_point potentialCursorPoint[width * height];
#endif
#endif

// OpenGL Variables
GLuint textureId;              // ID of the texture to contain Kinect RGB Data

// Kinect variables
IKinectSensor* sensor;         // Kinect sensor
#ifdef COLORCAMERA
ICoordinateMapper* mapper;
unsigned char rgbimage[colorwidth * colorheight * 4];
IMultiSourceFrameReader* reader;
ColorSpacePoint depth2rgb[width * height];
#else 
IDepthFrameReader* reader;     // Kinect depth data source

#endif

// for saving or retrieving file stream
#if defined(PUTBACKGROUNDDEPTH)
int getDepthFrequencyNumOfSample = 20;
int getDepthFrequency = 0;
int getDepthFrequencyBase = 100;
#endif
#ifdef RETRIEVEBACKGROUNDDEPTH
int getDepthFrequencyNumOfSample = 20;
unsigned short* startBgDepth;
CameraSpacePoint depth2xyzBackG[width * height];

#endif

void draw(void);

#ifdef WAVEBG
int drawingWaveBG() {

	while (1) {

		// Create a VideoCapture object and open the input file
		// If the input is the web camera, pass 0 instead of the video file name
		VideoCapture cap("Waves.mp4");

		// Check if camera opened successfully
		if (!cap.isOpened()) {
			cout << "Error opening video stream or file" << endl;
			return -1;
		}

		while (1) {

			//Mat BGframe;
			// Capture frame-by-frame 
			cap >> BGframe;

			// If the frame is empty, break immediately
			if (BGframe.empty())
				break;

			//intensity = BGframe.at<Vec3b>(130, 150);
			//uchar blue = intensity.val[0];
			//uchar green = intensity.val[1];
			//uchar red = intensity.val[2];
			//printf("B: %u, G: %u, R: %u \n", blue, green, red);

			// Display the resulting frame
			//imshow("Frame", BGframe);

			// Press  ESC on keyboard to exit
			char c = (char)waitKey(25);
			if (c == 27) {
				breakLoops = true;
				break;
			}

		}

		// When everything done, release the video capture object
		cap.release();
		if (breakLoops)
			break;

	}

	// Closes all the frames
	destroyAllWindows();

	return 0;
}
#endif

bool init(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
#if defined(LIMITVIEWRANGE)
	glutInitWindowSize(windowsWidth, windowsHeight);
#else
	glutInitWindowSize(width, height);
#endif
	glutCreateWindow("Kinect SDK Tutorial");
	//glutFullScreen();
	glutDisplayFunc(draw);
	glutIdleFunc(draw);
	return true;
}

GLvoid ResetWindowSize(GLvoid)  // Reset WindowSize
{
#if defined(LIMITVIEWRANGE)
	glutInitWindowSize(windowsWidth, windowsHeight);
	glutReshapeWindow(windowsWidth, windowsHeight);
#else
	glutInitWindowSize(width, height);
	glutReshapeWindow(width, height);
#endif
	glutPositionWindow(0, 0);
}

GLvoid SpecKey(GLint key, GLint x, GLint y)
{
	if (key == GLUT_KEY_F1)     // Toggle FullScreen
	{
		fullscreen = !fullscreen; // Toggle FullScreenBool
		if (fullscreen)
		{
			glutFullScreen();     // Enable Fullscreen
		}
		else
		{
			ResetWindowSize();    // Make Window, and resize
		}
	}
#ifdef WATERFLOW
	if (key == GLUT_KEY_F2)
	{
		_shouldCleanWaterDrop = true;
	}
#endif
#ifdef CURSOR
	if (key == GLUT_KEY_F3)
	{
		_shouldCleanCursorMark = true;
	}
#endif
}

#ifdef WATERFLOW
void clearWaterDropArray() {
#if !defined(LIMITVIEWRANGE)
	memset(waterDropArray, 0, sizeof(waterDropArray));
	//waterDropArray[Width * Height] = { 0 };
#else
	memset(waterDropArray, 0, sizeof(waterDropArray));
	//waterDropArray[limitedWidth * limitedHeight] = { 0 };
#endif
	_shouldCleanWaterDrop = false;
}
#endif

#ifdef CURSOR
void clearCursorMarkArray() {
#if !defined(LIMITVIEWRANGE)
	memset(blackColouredArray, 0, sizeof(blackColouredArray));
	//waterDropArray[Width * Height] = { 0 };
#else
	memset(blackColouredArray, 0, sizeof(blackColouredArray));
	//waterDropArray[limitedWidth * limitedHeight] = { 0 };
#endif
	_shouldCleanCursorMark = false;
}
#endif

bool initKinect() {
	if (FAILED(GetDefaultKinectSensor(&sensor))) {
		return false;
	}
	if (sensor) {
		sensor->Open();

#if defined(COLORCAMERA)
		sensor->get_CoordinateMapper(&mapper);
		sensor->OpenMultiSourceFrameReader(
			FrameSourceTypes::FrameSourceTypes_Depth | FrameSourceTypes::FrameSourceTypes_Color,
			&reader
		);
		return reader;
#else

		IDepthFrameSource* framesource = NULL;
		sensor->get_DepthFrameSource(&framesource);
		framesource->OpenReader(&reader);
		if (framesource) {
			framesource->Release();
			framesource = NULL;
		}
		return true;
#endif
	}
	else {
		return false;
	}
}

void getDepthData(IMultiSourceFrame* frame, GLubyte* dest) {
	IDepthFrameReference* frameref = NULL;
	IDepthFrame* depthframe;
	frame->get_DepthFrameReference(&frameref);
	frameref->AcquireFrame(&depthframe);
	if (frameref) frameref->Release();

	if (!depthframe) return;

	unsigned int sz;
	unsigned short* buf;
	depthframe->AccessUnderlyingBuffer(&sz, &buf);

	const unsigned short* start = (const unsigned short*)buf;

#ifdef PUTBACKGROUNDDEPTH
	if ((getDepthFrequency++ % getDepthFrequencyBase) == 0) {
		int numOfSample = ((getDepthFrequency - 1) / getDepthFrequencyBase) % getDepthFrequencyNumOfSample;
		char numOfSampleString[3];
		sprintf_s(numOfSampleString, "%d", numOfSample);
		char nameOfSample1[21] = "./background";
		char nameOfSample2[5] = ".bin";
		strcat_s(nameOfSample1, numOfSampleString);
		strcat_s(nameOfSample1, nameOfSample2);
		ofstream outputBackgroundDepth;
		printf("Start writing backgroundDepth, do not stop the program...\n");
		outputBackgroundDepth.open(nameOfSample1, ios::out | ios::binary);
		outputBackgroundDepth.seekp(0);
		outputBackgroundDepth.write((char*)start, sizeof(unsigned short) * height * width);
		if (outputBackgroundDepth.good()) {
			//printf("Write backgroundDepth file successfully\n");
			printf("Write backgroundDepth file background%d successfully\n", numOfSample);
		}
		else {
			printf("Failed to write backgroundDepth file\n");
		}
		outputBackgroundDepth.close();
	}
#endif

#ifdef LIMITVIEWRANGE
	for (int j = limitedOffsetY; j < (limitedHeight + limitedOffsetY); ++j) {
		for (int i = limitedOffsetX; i < (limitedWidth + limitedOffsetX); ++i) {
#else 
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
#endif

			const unsigned short* curr = start + ((width - i - 1) + width * j);
			unsigned short depth = (*curr);

#ifdef RETRIEVEBACKGROUNDDEPTH
			const unsigned short* currBackgroundDepth = startBgDepth + ((width - i - 1) + width * j);
			unsigned short depthBackgroundDepth = (*currBackgroundDepth);
			short objectHeight = (depthBackgroundDepth)-depth;

#ifdef CURSOR
#ifdef LIMITVIEWRANGE
			currentObjectHeightCacheForCursor[(limitedWidth - (i - limitedOffsetX) - 1) + limitedWidth * (j - limitedOffsetY)] = objectHeight>=0? objectHeight:0;
#else 
			currentObjectHeightCacheForCursor[(width - i - 1) + width * j] = objectHeight;
#endif
#endif

#ifdef WATERFLOW
#ifdef LIMITVIEWRANGE
			currentObjectHeightCache[(limitedWidth - (i - limitedOffsetX) - 1) + limitedWidth * (j - limitedOffsetY)] = objectHeight >= 0 ? objectHeight : 0;
#else 
			currentObjectHeightCache[(width - i - 1) + width * j] = objectHeight;
#endif
#endif

			if ( objectHeight <= -10) {
#ifdef WAVEBG
				if (BGframe.size != 0) {
					try {
						BGframeCopy = BGframe;
						intensity = BGframeCopy.at<Vec3b>(j, i);
						uchar blue = intensity.val[0];
						uchar green = intensity.val[1];
						uchar red = intensity.val[2];
						//					//printf("B: %u, G: %u, R: %u \n", blue, green, red);
						*dest++ = (BYTE)blue;
						*dest++ = (BYTE)green;
						*dest++ = (BYTE)red;
						*dest++ = 0xff;
					}
					catch (exception& e) {
						cout << e.what() << '\n';
						for (int i = 0; i < 3; ++i) {
							if (i == 0) {
								*dest++ = (BYTE)255;
							}
							else {
								*dest++ = (BYTE)0;
							}
						}
						*dest++ = 0xff;
					}
				}
				else {
					for (int i = 0; i < 3; ++i) {
						if (i == 2) {
							*dest++ = (BYTE)255;
						}
						else {
							*dest++ = (BYTE)0;
						}
					}
					*dest++ = 0xff;
				}
#else
				for (int i = 0; i < 3; ++i) {
					if (i == 0) {
						*dest++ = (BYTE)0;
					}
					else if (i == 1) {
						*dest++ = (BYTE)255;
					}
					else if (i == 2) {
						*dest++ = (BYTE)255;
					}
					else {
						*dest++ = (BYTE)0;
					}
				}
				*dest++ = 0xff;
				//printf("Red spot is at x: %d, y: %d \n", j%width, j / width);
#endif

			}
			else if (objectHeight > -10 && objectHeight <= 10) {
				for (int i = 0; i < 3; ++i) {
					if (i == 0) {
						*dest++ = (BYTE)0;
					}
					else if (i == 1) {
						*dest++ = (BYTE)0;
					}
					else if (i == 2) {
						*dest++ = (BYTE)255;
					}
					else {
						*dest++ = (BYTE)0;
					}
				}
				*dest++ = 0xff;
				//printf("Red spot is at x: %d, y: %d \n", j%width, j / width);
			}
				
			else if (objectHeight > 20) {
				for (int i = 0; i < 3; ++i) {
					if (i == 0) {
						*dest++ = (BYTE)255;
					}
					else if (i == 1) {
						*dest++ = (BYTE)255;
					}
					else if (i == 2) {
						*dest++ = (BYTE)255;
					}
					else {
						*dest++ = (BYTE)0;
					}
				}
				*dest++ = 0xff;
			//printf("Red spot is at x: %d, y: %d \n", j%width, j / width);
			}

			else {
#ifdef WAVEBG
				if (BGframe.size != 0) {
					try {
						BGframeCopy = BGframe;
						intensity = BGframeCopy.at<Vec3b>(j, i);
						uchar blue = intensity.val[0];
						uchar green = intensity.val[1];
						uchar red = intensity.val[2];
						//					//printf("B: %u, G: %u, R: %u \n", blue, green, red);
						*dest++ = (BYTE)blue;
						*dest++ = (BYTE)green;
						*dest++ = (BYTE)red;
						*dest++ = 0xff;
					}
					catch (exception& e) {
						cout << e.what() << '\n';
						for (int i = 0; i < 3; ++i) {
							if (i == 0) {
								*dest++ = (BYTE)255;
							}
							else {
								*dest++ = (BYTE)0;
							}
						}
						*dest++ = 0xff;
					}
				}
				else {
					for (int i = 0; i < 3; ++i) {
						if (i == 2) {
							*dest++ = (BYTE)255;
						}
						else {
							*dest++ = (BYTE)0;
						}
					}
					*dest++ = 0xff;
				}
				/*for (int i = 0; i < 3; ++i)
					* dest++ = (BYTE)depth % 256;
				*dest++ = 0xff;*/
#else
				for (int i = 0; i < 3; ++i) {
					if (i == 0) {
						*dest++ = (BYTE)0;
					}
					else if (i == 1) {
						*dest++ = (BYTE)255;
					}
					else if (i == 2) {
						*dest++ = (BYTE)255;
					}
					else {
						*dest++ = (BYTE)0;
					}
				}
				*dest++ = 0xff;
#endif
			}
#else

			// Draw a grayscale image of the depth:
			// B,G,R are all set to depth%256, alpha set to 1.
			if (depth < 1290 && depth > 1100) {
#ifdef WAVEBG
				if (BGframe.size != 0) {
					try {
						BGframeCopy = BGframe;
						intensity = BGframeCopy.at<Vec3b>(j, i);
						uchar blue = intensity.val[0];
						uchar green = intensity.val[1];
						uchar red = intensity.val[2];
						//					//printf("B: %u, G: %u, R: %u \n", blue, green, red);
						*dest++ = (BYTE)blue;
						*dest++ = (BYTE)green;
						*dest++ = (BYTE)red;
						*dest++ = 0xff;
					}
					catch (exception& e) {
						//cout << e.what() << '\n';
						for (int i = 0; i < 3; ++i) {
							if (i == 0) {
								*dest++ = (BYTE)255;
							}
							else {
								*dest++ = (BYTE)0;
							}
						}
						*dest++ = 0xff;
					}
				}
				else
				{
					for (int i = 0; i < 3; ++i) {
						if (i == 2) {
							*dest++ = (BYTE)255;
						}
						else {
							*dest++ = (BYTE)0;
						}
					}
					*dest++ = 0xff;
				}
#else
				for (int i = 0; i < 3; ++i) {
					if (i == 2) {
						*dest++ = (BYTE)255;
					}
					else {
						*dest++ = (BYTE)0;
					}
				}
				*dest++ = 0xff;
				//printf("Red spot is at x: %d, y: %d \n", j%width, j / width);
#endif
			}
			else {
				for (int i = 0; i < 3; ++i)
					* dest++ = (BYTE)depth % 256;
				*dest++ = 0xff;
			}
#endif


		}
	}

	mapper->MapDepthFrameToColorSpace(width* height, buf, width* height, depth2rgb);
	if (depthframe) depthframe->Release();
	
}

void getRgbData(IMultiSourceFrame* frame) {
	IColorFrame* colorframe;
	IColorFrameReference* frameref = NULL;
	frame->get_ColorFrameReference(&frameref);
	frameref->AcquireFrame(&colorframe);
	if (frameref) frameref->Release();

	if (!colorframe) return;

	colorframe->CopyConvertedFrameDataToArray(colorwidth * colorheight * 4, rgbimage, ColorImageFormat_Bgra);

	if (colorframe) colorframe->Release();

}

#ifdef MORPHOLOGY
#ifdef LIMITVIEWRANGE
void OpenCVMorphology(GLubyte* dest) {
	int morph_elem = 0; //0: Rect  1: Cross  2: Ellipse
	int morph_size = 1;
	destCastInMatsrc = Mat(Size(limitedWidth, limitedHeight), CV_8UC4, dest, cv::Mat::AUTO_STEP);
	Mat converted;
	cvtColor(destCastInMatsrc, converted, COLOR_BGRA2BGR);
	Mat element = getStructuringElement(morph_elem, Size(2 * morph_size + 1, 2 * morph_size + 1), Point(morph_size, morph_size));
	morphologyEx(converted, destCastInMatdst, MORPH_CLOSE, element);
	imshow("MorpOutPut", destCastInMatsrc);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, limitedWidth, limitedHeight, GL_BGRA_EXT, GL_UNSIGNED_BYTE, destCastInMatdst.ptr());
	for (int j = 0; j < limitedHeight; ++j) {
		for (int i = 0; i < limitedWidth; ++i) {
			if (destCastInMatdst.size != 0) {
				try {
					intensity = destCastInMatdst.at<Vec3b>(j, i);
					uchar blue = intensity.val[0];
					uchar green = intensity.val[1];
					uchar red = intensity.val[2];
					//					//printf("B: %u, G: %u, R: %u \n", blue, green, red);
					if (red == 255 && blue == 0 && green == 0) {
						ColorSpacePoint p = depth2rgb[(width - limitedWidth - limitedOffsetX + (limitedWidth - i - 1)) + (width * (limitedOffsetY + j))];
						int idx = (int)p.X + colorwidth * (int)p.Y;
						uchar rbgImageBlue = rgbimage[4 * idx + 0];
						uchar rbgImageGreen = rgbimage[4 * idx + 1];
						uchar rbgImageRed = rgbimage[4 * idx + 2];
						if (!(((rbgImageRed - rbgImageGreen) < 100 && (rbgImageRed - rbgImageGreen) > 20) && ((rbgImageGreen - rbgImageBlue) < 100 && (rbgImageGreen - rbgImageBlue) > -10))) {
							*dest++ = (BYTE)0;
							*dest++ = (BYTE)255;
							*dest++ = (BYTE)255;
							*dest++ = 0xff;
							/**dest++ = (BYTE)rbgImageBlue;
							*dest++ = (BYTE)rbgImageGreen;
							*dest++ = (BYTE)rbgImageRed;
							*dest++ = 0xff;*/
						}
						else {
							*dest++ = (BYTE)blue;
							*dest++ = (BYTE)green;
							*dest++ = (BYTE)red;
							*dest++ = 0xff;
							/**dest++ = (BYTE)rbgImageBlue;
							*dest++ = (BYTE)rbgImageGreen;
							*dest++ = (BYTE)rbgImageRed;
							*dest++ = 0xff;*/
						}

					}
					else {
						*dest++ = (BYTE)blue;
						*dest++ = (BYTE)green;
						*dest++ = (BYTE)red;
						*dest++ = 0xff;
					}
					
				}
				catch (exception & e) {
					cout << e.what() << '\n';
					for (int i = 0; i < 3; ++i) {
						if (i == 0) {
							*dest++ = (BYTE)255;
						}
						else {
							*dest++ = (BYTE)0;
						}
					}
					*dest++ = 0xff;
				}
			}
			else {
				for (int i = 0; i < 3; ++i) {
					if (i == 2) {
						*dest++ = (BYTE)255;
					}
					else {
						*dest++ = (BYTE)0;
					}
				}
				*dest++ = 0xff;
			}
		}
	}
}


#else
#endif
#endif

#if defined(WATERFLOW) || defined(CURSOR)
int notFewerThanZero(int number) {
	if (number < 0) {
		return 00;
	}
	else {
		return number;
	}
}

int notHigherThanWidthLimit(int number) {
#ifdef LIMITVIEWRANGE
	if (number >= limitedWidth - 1) {
		return limitedWidth - 1;
	}
	else {
		return number;
	}
#else
	if (number >= width - 1) {
		return width - 1;
	}
	else {
		return number;
	}
#endif
}

int notHigherThanHeightLimit(int number) {
#ifdef LIMITVIEWRANGE
	if (number >= limitedHeight -1) {
		return limitedHeight - 1;
	}
	else {
		return number;
	}
#else
	if (number >= height -1) {
		return height -1;
	}
	else {
		return number;
	}
#endif
}

int findSmallestElement(int arr[], int n) {
	/* We are assigning the first array element to
	 * the temp variable and then we are comparing
	 * all the array elements with the temp inside
	 * loop and if the element is smaller than temp
	 * then the temp value is replaced by that. This
	 * way we always have the smallest value in temp.
	 * Finally we are returning temp.
	 */
	int temp = arr[0];
	int sequence = 0;
	for (int i = 0; i < n; i++) {
		if (temp >= arr[i]) {
			temp = arr[i];
			sequence = i;
		}
	}
	return sequence;
}

int sum(int arr[], int n)
{
	int sum = 0; // initialize sum  

	// Iterate through all elements  
	// and add them to sum  
	for (int i = 0; i < n; i++)
		sum += arr[i];

	return sum;
}

int matrixPositionInArray(int x, int y, int position) {
#ifdef LIMITVIEWRANGE
	switch (position)
	{
	case 0:
		return notFewerThanZero(x - 1) + limitedWidth * notFewerThanZero(y - 1);
	case 1:
		return notFewerThanZero(x) + limitedWidth * notFewerThanZero(y - 1);
	case 2:
		return notHigherThanWidthLimit(notFewerThanZero(x + 1)) + limitedWidth * notFewerThanZero(y - 1);
	case 3:
		return notFewerThanZero(x - 1) + limitedWidth * notFewerThanZero(y);
	case 4:
		return notFewerThanZero(x) + limitedWidth * notFewerThanZero(y);
	case 5:
		return notHigherThanWidthLimit(notFewerThanZero(x + 1)) + limitedWidth * notFewerThanZero(y);
	case 6:
		return notFewerThanZero(x - 1) + limitedWidth * notHigherThanHeightLimit( notFewerThanZero(y + 1));
	case 7:
		return notFewerThanZero(x) + limitedWidth * notHigherThanHeightLimit(notFewerThanZero(y + 1));
	case 8:
		return notHigherThanWidthLimit(notFewerThanZero(x + 1)) + limitedWidth * notHigherThanHeightLimit( notFewerThanZero(y + 1));
	default:
		break;
	}
#else
	switch (position)
	{
	case 0:
		return notFewerThanZero(x - 1) + width * notFewerThanZero(y - 1);
	case 1:
		return notFewerThanZero(x) + width * notFewerThanZero(y - 1);
	case 2:
		return notHigherThanWidthLimit(notFewerThanZero(x + 1)) + width * notFewerThanZero(y - 1);
	case 3:
		return notFewerThanZero(x - 1) + width * notFewerThanZero(y);
	case 4:
		return notFewerThanZero(x) + width * notFewerThanZero(y);
	case 5:
		return notHigherThanWidthLimit(notFewerThanZero(x + 1)) + width * notFewerThanZero(y);
	case 6:
		return notFewerThanZero(x - 1) + width * notHigherThanHeightLimit(notFewerThanZero(y + 1));
	case 7:
		return notFewerThanZero(x) + width * notHigherThanHeightLimit(notFewerThanZero(y + 1));
	case 8:
		return notHigherThanWidthLimit(notFewerThanZero(x + 1)) + width * notHigherThanHeightLimit(notFewerThanZero(y + 1));
	default:
		break;
	}
#endif

}
#endif

#if defined(WATERFLOW)

void addWaterDrop(GLubyte * dest) {
#ifdef LIMITVIEWRANGE
	for (int j = 0; j < limitedHeight; ++j) {
		for (int i = 0; i < limitedWidth; ++i) {
			if (currentObjectHeightCache[(limitedWidth - 1 - i) + limitedWidth * j] > 200 && currentObjectHeightCache[(limitedWidth - 1 - i) + limitedWidth * j] < 300) {
				waterDropArray[(limitedWidth - 1 - i) + limitedWidth * j] += 1;
			}
		}
	}

	for (int j = 0; j < limitedHeight; ++j) {
		for (int i = 0; i < limitedWidth; ++i) {
			//int dropArray = waterDropArray[(limitedWidth - 1 - i) + limitedWidth * j];
			/*for (int k = 0; k < dropArray; ++k) {
				int depthMatrix[9] = { currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 0)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 0)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 1)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 1)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 2)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 2)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 3)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 3)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 4)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 4)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 5)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 5)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 6)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 6)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 7)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 7)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 8)] + waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 8)] };
				int lowestHeightPosition = findSmallestElement(depthMatrix, 9);
				waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 4)] -= 1;
				waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, lowestHeightPosition)] += 1;
			}*/
				int depthMatrixWithoutWater[9] = { currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1- i), j, 0)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 1)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 2)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 3)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 4)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 5)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 6)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 7)],
					currentObjectHeightCache[matrixPositionInArray((limitedWidth - 1 - i), j, 8)] };
				int depthMatrixWaterDrop[9] = { waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 0)],
					waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 1)],
					waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 2)],
					waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 3)],
					waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 4)],
					waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 5)],
					waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 6)],
					waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 7)],
					waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 8)] };
				int whichMatrixRemoved[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
				int averageHeight = 0;
				bool isMatrixRemoved = false;
				bool isWaterDropDistributed = false;
				while(!isWaterDropDistributed) {
					isMatrixRemoved = false;
					int sumWhichMatrixRemoved = sum(whichMatrixRemoved, 9);
					averageHeight = (sum(depthMatrixWithoutWater, 9) + sum(depthMatrixWaterDrop, 9)) / (sumWhichMatrixRemoved <= 0 ? 1 : sumWhichMatrixRemoved);
					for (int i = 0; i < 9; i++) {
						if (averageHeight < depthMatrixWithoutWater[i]) {
							depthMatrixWithoutWater[i] = 0;
							isMatrixRemoved = true;
							whichMatrixRemoved[i] = 0;
						}
					}
					if (!isMatrixRemoved) {
						int sumUpWaterDropDistributed = 0;
						for (int k = 0; k < 9; k++) {
							if (whichMatrixRemoved[k] == 1) {
								waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, k)] = averageHeight - depthMatrixWithoutWater[k];
								sumUpWaterDropDistributed += averageHeight - depthMatrixWithoutWater[k];
							}
							else {
								waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, k)] = 0;
							}
						}
						int remainingWaterDrop = sum(depthMatrixWaterDrop, 9) - sumUpWaterDropDistributed;
						waterDropArray[matrixPositionInArray((limitedWidth - 1 - i), j, 1)] += remainingWaterDrop >= 0 ? remainingWaterDrop : 0;
						isWaterDropDistributed = true;
					}
				}
			
		}
	}

	for (int j = 0; j < limitedHeight; ++j) {
		for (int i = 0; i < limitedWidth; ++i) {
			int numberOfDrop = waterDropArray[(limitedWidth - 1 - i) + limitedWidth * j];
			for (int i = 0; i < 3; ++i) {
				if (i == 0) {
					int colourHexInDec = (int)* dest;
					int increment = (255 - colourHexInDec) / 20;
					int newColourHexInDec = colourHexInDec + (increment * numberOfDrop);
					*dest++ = (BYTE)(newColourHexInDec > 255 ? 255 : newColourHexInDec);
				}
				else if (i == 1) {
					int colourHexInDec = (int)* dest;
					int increment = (colourHexInDec) / 20;
					int newColourHexInDec = increment * (20 - numberOfDrop);
					*dest++ = (BYTE)(newColourHexInDec < 0 ? 0 : newColourHexInDec);
				}
				else if (i == 2) {
					int colourHexInDec = (int)* dest;
					int increment = (colourHexInDec) / 20;
					int newColourHexInDec = increment * (20 - numberOfDrop);
					*dest++ = (BYTE)(newColourHexInDec < 0 ? 0 : newColourHexInDec);
				}
				else {
					*dest++ = (BYTE)0;
				}
			}
			*dest++ = 0xff;
		}
	}
#else
	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			if (currentObjectHeightCache[(width - i) + width * j] > 200 && currentObjectHeightCache[(width - i) + width * j] < 300) {
				waterDropArray[(width - i) + width * j] += 1;
			}
		}
	}

	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			for (int k = 0; k < waterDropArray[(width -1- i) + width * j]; ++k) {
				int depthMatrix[9] = { currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 0)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 0)],
					currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 1)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 1)],
					currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 2)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 2)],
					currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 3)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 3)],
					currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 4)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 4)],
					currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 5)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 5)],
					currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 6)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 6)],
					currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 7)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 7)],
					currentObjectHeightCache[matrixPositionInArray((width -1 - i), j, 8)] + waterDropArray[matrixPositionInArray((width -1 - i), j, 8)] };
				int lowestHeightPosition = findSmallestElement(depthMatrix, 9);
				waterDropArray[matrixPositionInArray((width -1 - i), j, 4)] -= 1;
				waterDropArray[matrixPositionInArray((width -1 - i), j, lowestHeightPosition)] += 1;

			}
		}
	}

	for (int j = 0; j < height; ++j) {
		for (int i = 0; i < width; ++i) {
			int numberOfDrop = waterDropArray[(width - 1 - i) + width * j];
			for (int i = 0; i < 3; ++i) {
				if (i == 0) {
					int colourHexInDec = (int)* dest;
					int increment = (255 - colourHexInDec) / 20;
					int newColourHexInDec = colourHexInDec + (increment * numberOfDrop);
					*dest++ = (BYTE)(newColourHexInDec > 255 ? 255 : newColourHexInDec);
				}
				else if (i == 1) {
					int colourHexInDec = (int)* dest;
					int increment = (colourHexInDec) / 20;
					int newColourHexInDec = increment * (20 - numberOfDrop);
					*dest++ = (BYTE)(newColourHexInDec < 0 ? 0 : newColourHexInDec);
				}
				else if (i == 2) {
					int colourHexInDec = (int)* dest;
					int increment = (colourHexInDec) / 20;
					int newColourHexInDec = increment * (20 - numberOfDrop);
					*dest++ = (BYTE)(newColourHexInDec < 0 ? 0 : newColourHexInDec);
				}
				else {
					*dest++ = (BYTE)0;
				}
			}
			*dest++ = 0xff;
		}
	}

#endif
}
#endif

#ifdef CURSOR
void setCursorPoint(GLubyte* dest) {

#if !defined(LIMITVIEWRANGE)
	if (objectHeight > 109 && objectHeight <= 110)
	{
		potentialCursorPoint[(width - 1 - i) + width * j] = chrono::system_clock::now();
	}
	if (objectHeight > 0 && objectHeight <= 1)
	{
		std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - potentialCursorPoint[(width - 1 - i) + width * j];
		if (elapsed_seconds.count() > 0.1 && elapsed_seconds.count() < 0.3)
		{
			//std::time_t end_time = std::chrono::system_clock::to_time_t(potentialCursorPoint[(width - 1 - i) + width * j]);
			//cout << "finished computation at " << std::ctime_s(&end_time);
			//cout << "\n";
			printf("x: %u, y: %u \n", i, j);
		}
	}
#else 

	for (int j = 0; j < limitedHeight; ++j) {
		for (int i = 0; i < limitedWidth; ++i) {
			if (currentObjectHeightCacheForCursor[(limitedWidth - 1 - i) + limitedWidth * j] > 19 && currentObjectHeightCacheForCursor[(limitedWidth - 1 - i) + limitedWidth * j] <= 20) {
				blackColouredArray[(limitedWidth - 1 - i) + limitedWidth * j] += 1;
			}
		}
	}

	//for (int j = 0; j < limitedHeight; ++j) {
	//	for (int i = 0; i < limitedWidth; ++i) {
	//		bool isTargeted = false;
	//		bool isConfirmed = false;
	//		if (currentObjectHeightCacheForCursor[(limitedWidth - 1 - i) + limitedWidth * j] > 19 && currentObjectHeightCacheForCursor[(limitedWidth - 1 - i) + limitedWidth * j] <= 20) {
	//			for (int k = 0; k < 9; k++) {
	//				if (!(currentObjectHeightCacheForCursor[matrixPositionInArray((limitedWidth - 1 - i), j, k)] > 19) || !(currentObjectHeightCacheForCursor[matrixPositionInArray((limitedWidth - 1 - i), j, k)]<= 20)) {
	//					break;
	//				}
	//				isTargeted = true;
	//			}
	//		}
	//		if (isTargeted) {
	//			potentialCursorPoint[(limitedWidth - 1 - i) + limitedWidth * j] = chrono::system_clock::now();
	//		}

	//		if (currentObjectHeightCacheForCursor[(limitedWidth - 1 - i) + limitedWidth * j] <= 1)
	//		{
	//			std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - potentialCursorPoint[(limitedWidth - 1 - (i)) + limitedWidth * (j)];
	//			if (elapsed_seconds.count() > 0.0 && elapsed_seconds.count() < 0.3)
	//			{
	//				for (int k = 0; k < 9; k++) {
	//					std::chrono::duration<double> elapsed_seconds_kernel = std::chrono::system_clock::now() - potentialCursorPoint[matrixPositionInArray((limitedWidth - 1 - i), j, k)];
	//					if (!(currentObjectHeightCacheForCursor[matrixPositionInArray((limitedWidth - 1 - i), j, k)] <= 1) && (elapsed_seconds_kernel.count() > 0.0 && elapsed_seconds_kernel.count() < 0.3)) {
	//						break;
	//					}
	//					isConfirmed = true;
	//				}
	//			}
	//		}
	//		if (isConfirmed) {
	//			
	//				//printf("x: %u, y: %u \n", (i), (j));

	//			blackColouredArray[(limitedWidth - 1 - i) + limitedWidth * j] += 1;
	//				
	//		}
	//	}
	//}

	for (int j = 0; j < limitedHeight; ++j) {
		for (int i = 0; i < limitedWidth; ++i) {
			int numberOfDrop = blackColouredArray[(limitedWidth - 1 - i) + limitedWidth * j];
			
				for (int l = 0; l < 3; l++) {
					if (l == 0 && numberOfDrop >= 250) {
						int colourHexInDec = (int)*dest;
						int increment = (255 - colourHexInDec) / 100;
						int newColourHexInDec = colourHexInDec + (increment * numberOfDrop);
						//dest[(limitedWidth - 1 - i) + limitedWidth * j] = (BYTE)(newColourHexInDec > 255 ? 255 : newColourHexInDec);
						//dest++;
						*dest++ = (BYTE)(newColourHexInDec > 255 ? 255 : newColourHexInDec);
					}
					else if (l == 1 && numberOfDrop >= 250) {
						int colourHexInDec = (int)*dest;
						int increment = (colourHexInDec) / 100;
						int newColourHexInDec = increment * (100 - numberOfDrop);
						*dest++ = (BYTE)(newColourHexInDec < 0 ? 0 : newColourHexInDec);
					}
					else if (l == 2 && numberOfDrop >= 250) {
						int colourHexInDec = (int)*dest;
						int increment = (colourHexInDec) / 100;
						int newColourHexInDec = increment * (100 - numberOfDrop);
						*dest++ = (BYTE)(newColourHexInDec < 0 ? 0 : newColourHexInDec);
					}
					else if (numberOfDrop >= 250){
						*dest++ = (BYTE)0;
					}
					else {
						*dest++;
					}
				}
				*dest++ = 0xff;
			
			
		}
	}

	//for (int j = 0; j < limitedHeight; ++j) {
	//	for (int i = 0; i < limitedWidth; ++i) {
	//		if (blackColouredArray[(limitedWidth - 1 - i) + limitedWidth * j] == 0) {
	//			for (int k = 0; k < 9; k++) {
	//				for (int l = 0; l < 3; ++l) {
	//					if (l == 0) {
	//						dest[matrixPositionInArray(i, j, k)] = (BYTE)255;
	//						dest++;
	//					}
	//					else if (l == 1) {
	//						*dest++ = (BYTE)255;
	//					}
	//					else if (l == 2) {
	//						*dest++ = (BYTE)255;
	//					}
	//					else {
	//						*dest++ = (BYTE)0;
	//					}
	//				}
	//				*dest++ = 0xff;
	//			}
	//		}
	//		
	//	}
	//}
	

	//for (int j = 0; j < height; ++j) {
//	for (int i = 0; i < width; ++i) {
//		int numberOfDrop = waterDropArray[(width - 1 - i) + width * j];
//		for (int i = 0; i < 3; ++i) {
//			if (i == 0) {
//				int colourHexInDec = (int)*dest;
//				int increment = (255 - colourHexInDec) / 20;
//				int newColourHexInDec = colourHexInDec + (increment * numberOfDrop);
//				*dest++ = (BYTE)(newColourHexInDec > 255 ? 255 : newColourHexInDec);
//			}
//			else if (i == 1) {
//				int colourHexInDec = (int)*dest;
//				int increment = (colourHexInDec) / 20;
//				int newColourHexInDec = increment * (20 - numberOfDrop);
//				*dest++ = (BYTE)(newColourHexInDec < 0 ? 0 : newColourHexInDec);
//			}
//			else if (i == 2) {
//				int colourHexInDec = (int)*dest;
//				int increment = (colourHexInDec) / 20;
//				int newColourHexInDec = increment * (20 - numberOfDrop);
//				*dest++ = (BYTE)(newColourHexInDec < 0 ? 0 : newColourHexInDec);
//			}
//			else {
//				*dest++ = (BYTE)0;
//			}
//		}
//		*dest++ = 0xff;
//	}
//}

#endif
}
#endif

void getKinectData(GLubyte* dest) {
	IMultiSourceFrame* frame = NULL;
	if (SUCCEEDED(reader->AcquireLatestFrame(&frame))) {
		getDepthData(frame, dest);
		getRgbData(frame);
		
	}
	if (frame) frame->Release();
}

void drawKinectData() {
#ifdef LIMITVIEWRANGE
	glBindTexture(GL_TEXTURE_2D, textureId);
	getKinectData(glBytedata);
#ifdef CURSOR
	if (_shouldCleanCursorMark) {
		clearCursorMarkArray();
	}
	setCursorPoint(glBytedata);
#endif
#ifdef WATERFLOW
	if (_shouldCleanWaterDrop) {
		clearWaterDropArray();
	}
	addWaterDrop(glBytedata);
#endif
#ifdef MORPHOLOGY
#ifdef LIMITVIEWRANGE
	OpenCVMorphology(glBytedata);
#else
#endif
#endif
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, limitedWidth, limitedHeight, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)glBytedata);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0, 0, 0);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(limitedWidth, 0, 0);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(limitedWidth, limitedHeight, 0.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0, limitedHeight, 0.0f);
	glEnd();
#else
	glBindTexture(GL_TEXTURE_2D, textureId);
	getKinectData(glBytedata);
#ifdef CURSOR
	if (_shouldCleanCursorMark) {
		clearCursorMarkArray();
	}
	setCursorPoint(glBytedata);
#endif
#ifdef WATERFLOW
	if (_shouldCleanWaterDrop) {
		clearWaterDropArray();
	}
	addWaterDrop(glBytedata);
#endif
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)glBytedata);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0, 0, 0);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(width, 0, 0);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(width, height, 0.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0, height, 0.0f);
	glEnd();
#endif
}



void draw() {
	drawKinectData();
	glutSwapBuffers();
}

void execute() {
	glutMainLoop();
}

int main(int argc, char* argv[]) {
	if (!init(argc, argv)) return 1;
	if (!initKinect()) return 1;
	glutSpecialFunc(SpecKey);

#ifdef CURSOR
#if !defined(LIMITVIEWRANGE)
	for (int i = 0; i < (width * height); i++) {
		potentialCursorPoint[i] = programStartUpTime;
	}
#else
	for (int i = 0; i < (limitedWidth * limitedHeight); i++) {
		potentialCursorPoint[i] = programStartUpTime;
	}
#endif
#endif

#ifdef LIMITVIEWRANGE
	// Initialize textures
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, limitedWidth, limitedHeight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)glBytedata);
	glBindTexture(GL_TEXTURE_2D, 0);

	// OpenGL setup
	glClearColor(0, 0, 0, 0);
	glClearDepth(1.0f);
	glEnable(GL_TEXTURE_2D);

	// Camera setup
	glViewport(0, 0, limitedWidth, limitedHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, limitedWidth, limitedHeight, 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#else 
	// Initialize textures
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)glBytedata);
	glBindTexture(GL_TEXTURE_2D, 0);

	// OpenGL setup
	glClearColor(0, 0, 0, 0);
	glClearDepth(1.0f);
	glEnable(GL_TEXTURE_2D);

	// Camera setup
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif


#if defined(RETRIEVEBACKGROUNDDEPTH)
	unsigned short* startBgDepth2 = new unsigned short[height * width];;
	startBgDepth = new unsigned short[height * width];
	for (int k = 0; k < getDepthFrequencyNumOfSample; k++) {
		int numOfSample = k;
		char numOfSampleString[3];
		sprintf_s(numOfSampleString, "%d", numOfSample);
		char nameOfSample1[21] = "./background";
		char nameOfSample2[5] = ".bin";
		strcat_s(nameOfSample1, numOfSampleString);
		strcat_s(nameOfSample1, nameOfSample2);
		ifstream inputBackgroundDepth(nameOfSample1, ios::in | ios::binary);
		if (inputBackgroundDepth.good()) {
			inputBackgroundDepth.seekg(0);
			//startBgDepth2 = new unsigned short[height * width];
			inputBackgroundDepth.read((char*)startBgDepth2, sizeof(unsigned short) * height * width);
			printf("Get backgroundDepth file background%d successfully\n", numOfSample);
			inputBackgroundDepth.close();
		}
		else {
			printf("Failed to get backgroundDepth file\n");
		}
		if (k == 0) {
			startBgDepth = startBgDepth2;
		}
		else {
			for (int j = 0; j < height; ++j) {
				for (int i = 0; i < width; ++i) {
					const unsigned short* currBackgroundDepth = startBgDepth + (i + width * j);
					unsigned short depthBackgroundDepth = (*currBackgroundDepth);
					const unsigned short* currBackgroundDepth2 = startBgDepth2 + (i + width * j);
					unsigned short depthBackgroundDepth2 = (*currBackgroundDepth2);
					*startBgDepth = (depthBackgroundDepth * k + depthBackgroundDepth2) / (k + 1);
				}
			}
		}
	}
	
#endif

#ifdef WAVEBG

	thread th1(drawingWaveBG);
	//if (!cap.isOpened()) {
	//	printf("Background mp4 has been opened");
	//}
#endif

	// Main loop
	execute();
	return 0;
}