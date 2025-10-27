#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

/**
 * Display an image using WxWidgets.
 * https://www.wxwidgets.org/
 */

/** Declarations*/
const int WIDTH = 352;
const int HEIGHT = 288;
const int DEBUG_X = 0;
const int DEBUG_Y = 0;

/**
 * Class that implements wxApp
 */
class MyApp : public wxApp {
 public:
  bool OnInit() override;
};

/**
 * Class that implements wxFrame.
 * This frame serves as the top level window for the program
 */
class MyFrame : public wxFrame {
 public:
  MyFrame(const wxString &title, string imagePath);
  MyFrame(const wxString &title, string imagePath, unsigned char *inData);

 private:
  void OnPaint(wxPaintEvent &event);
  wxImage inImage;
  wxScrolledWindow *scrolledWindow;
};

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height);
float DCT(int u, int v, unsigned char** f);
float IDCT(int x, int y, unsigned char** F);
float C(int k);

unsigned char*** convertToBlocks(unsigned char* imageData, int channel);
unsigned char* convertToImageData(unsigned char*** rBlocks, unsigned char*** gBlocks, unsigned char*** bBlocks);
void free3DArray(unsigned char*** blocks);


/** Definitions */

/**
 * Init method for the app.
 * Here we process the command line arguments and
 * instantiate the frame.
 */
bool MyApp::OnInit() {
  wxInitAllImageHandlers();

  // deal with command line arguments here
  cout << "Number of command line arguments: " << wxApp::argc << endl;
  if (wxApp::argc != 5) {
    cerr << "The executable should be invoked with exactly 5 arguments: "
            "./MyImageApplication InputImage quantizationLevel DeliveryMode Latency"
         << endl;
    exit(1);
  }
  cout << "First argument: " << wxApp::argv[0] << endl;
  cout << "Second argument: " << wxApp::argv[1] << endl;
  cout << "Third argument: " << wxApp::argv[2] << endl;
  cout << "Fourth argument: " << wxApp::argv[3] << endl;
  cout << "Fifth argument: " << wxApp::argv[4] << endl;
  string imagePath = wxApp::argv[1].ToStdString();
  int N = stoi(wxApp::argv[2].ToStdString()); // Quantization level, Range [0,7]
  int M = stoi(wxApp::argv[3].ToStdString()); // Delivery Mode, Range [1,3]
  int L = stoi(wxApp::argv[4].ToStdString()); // Latency in milliseconds

  unsigned char *inData = readImageData(imagePath, WIDTH, HEIGHT);
  //cout << "Original: " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 1]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 2]) << endl; 

  unsigned char*** rBlocks = convertToBlocks(inData, 0);
  unsigned char*** gBlocks = convertToBlocks(inData, 1);
  unsigned char*** bBlocks = convertToBlocks(inData, 2);
  free(inData);

  unsigned char *outData = convertToImageData(rBlocks,gBlocks,bBlocks);
  free3DArray(rBlocks);
  free3DArray(gBlocks);
  free3DArray(bBlocks);

  /*
  // C
  if (colorMode == 1) {
    if (quantizationMode == 1) {
      uniformQuantization(inData, q1, q2, q3);
    }
    else {
      nonUniformQuantization(inData, q1, q2, q3);
    }
  }
  if (colorMode == 2) {
    float *data = normalizeImageData(inData, WIDTH, HEIGHT);
    cout << "After normalization: " << data[3 * DEBUG_X * DEBUG_Y] << ", " << data[3 * DEBUG_X * DEBUG_Y + 1] << ", " << data[3 * DEBUG_X * DEBUG_Y + 2] << endl; 

    // RGB to YUV
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
      float y = 0.299f * data[3*i] + 0.587f * data[3*i+1] + 0.114f * data[3*i+2];
      float u = -0.147f * data[3*i] + -0.289f * data[3*i+1] + 0.436f * data[3*i+2];
      float v = 0.615f * data[3*i] + -0.515f * data[3*i+1] + -0.100f * data[3*i+2];
      data[3 * i] = y;
      data[3 * i + 1] = u;
      data[3 * i + 2] = v;
    }
    cout << "In YUV: " << data[0] << ", " << data[1] << ", " << data[2] << endl; 

    if (quantizationMode == 1) {
      uniformQuantization(data, q1, q2, q3);
    }
    else {
      nonUniformQuantizationF(data, q1, q2, q3);
    }
    cout << "In YUV after quantization: " << data[3 * DEBUG_X * DEBUG_Y] << ", " << data[3 * DEBUG_X * DEBUG_Y + 1] << ", " << data[3 * DEBUG_X * DEBUG_Y + 2] << endl; 

    // YUV to RGB
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
      data[3*i] = clamp<float>(data[3*i], 0.f, 1.f);
      data[3*i+1] = clamp<float>(data[3*i+1], -0.436f, 0.436f);
      data[3*i+2] = clamp<float>(data[3*i+2], -0.615f, 0.615f);

      float r = 1.000f * data[3*i] + 0.000f * data[3*i+1] + 1.1398f * data[3*i+2];
      float g = 1.000f * data[3*i] + -0.3946f * data[3*i+1] + -0.5806f * data[3*i+2];
      float b = 1.000f * data[3*i] + 2.0321f * data[3*i+1] + 0.f * data[3*i+2];
      // Prevent values from exceed 255 or going below 0 before converting to unsigned char
      inData[3 * i] = clamp<float>(r * 255, 0.f, 255.f);
      inData[3 * i + 1] = clamp<float>(g * 255, 0.f, 255.f);
      inData[3 * i + 2] = clamp<float>(b * 255, 0.f, 255.f);
    }
    free(data);
  }
  */

  //cout << "After processing: " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 1]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 2]) << endl; 
  MyFrame *frame = new MyFrame("Image Display", imagePath, outData);
  frame->Show(true);

  // return true to continue, false to exit the application
  return true;
}

float C(int k) 
{
  if (k == 0) {
    return 1.0f / sqrtf(k);
  }
  else {
    return 1.0f;
  }
}

float DCT(int u, int v, unsigned char** f) 
{
  float result = (1.0f/4.0f) * C(u) * C(v);
  float sum = 0.0f;
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      sum += f[x][y] * cosf(((2.0f*x + 1.0f) * u * M_PI) / 16.0f) * cosf(((2.0f*y + 1.0f) * v * M_PI) / 16.0f);
    }
  }

  return result * sum;
}

float IDCT(int x, int y, unsigned char** F)
{
  float result = 1.0f / 4.0f;
  float sum = 0.0f;
  for (int u = 0; u < 8; x++) {
    for (int v = 0; v < 8; y++) {
      sum += C(u) * C(v) * F[u][v] * cosf(((2.0f*x + 1.0f) * u * M_PI) / 16.0f) * cosf(((2.0f*y + 1.0f) * v * M_PI) / 16.0f);
    }
  }

  return result * sum;
}

/**
 * Constructor for the MyFrame class.
 * Here we read the pixel data from the file and set up the scrollable window.
 */
MyFrame::MyFrame(const wxString &title, string imagePath)
    : wxFrame(NULL, wxID_ANY, title) {

  // Modify the height and width values here to read and display an image with
  // different dimensions.    

  unsigned char *inData = readImageData(imagePath, WIDTH, HEIGHT);

  // the last argument is static_data, if it is false, after this call the
  // pointer to the data is owned by the wxImage object, which will be
  // responsible for deleting it. So this means that you should not delete the
  // data yourself.
  inImage.SetData(inData, WIDTH, HEIGHT, false);

  // Set up the scrolled window as a child of this frame
  scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
  scrolledWindow->SetScrollbars(10, 10, WIDTH, HEIGHT);
  scrolledWindow->SetVirtualSize(WIDTH, HEIGHT);

  // Bind the paint event to the OnPaint function of the scrolled window
  scrolledWindow->Bind(wxEVT_PAINT, &MyFrame::OnPaint, this);

  // Set the frame size
  SetClientSize(WIDTH, HEIGHT);

  // Set the frame background color
  SetBackgroundColour(*wxBLACK);
}

MyFrame::MyFrame(const wxString &title, string imagePath, unsigned char* inData)
    : wxFrame(NULL, wxID_ANY, title) {

  unsigned char *originalData = readImageData(imagePath, WIDTH, HEIGHT);
  unsigned char *mergedData =
      (unsigned char *)malloc(WIDTH * HEIGHT * 3 * 2 * sizeof(unsigned char));
  float error = 0;
      
  // Iterate row by row
  for (int row = 0; row < HEIGHT; row++) {
      for (int col = 0; col < WIDTH; col++) {
          // Left image
          int mergedIndex = (row * WIDTH * 2 + col) * 3;
          int originalIndex = (row * WIDTH + col) * 3;
          mergedData[mergedIndex]     = originalData[originalIndex];
          mergedData[mergedIndex + 1] = originalData[originalIndex + 1];
          mergedData[mergedIndex + 2] = originalData[originalIndex + 2];

          // Right image
          mergedIndex = (row * WIDTH * 2 + (col + WIDTH)) * 3;
          int inIndex = (row * WIDTH + col) * 3;
          mergedData[mergedIndex]     = inData[inIndex];
          mergedData[mergedIndex + 1] = inData[inIndex + 1];
          mergedData[mergedIndex + 2] = inData[inIndex + 2];

          error += abs(originalData[originalIndex] / 255.f - inData[inIndex] / 255.f);
          error += abs(originalData[originalIndex+1] / 255.f - inData[inIndex+1] / 255.f);
          error += abs(originalData[originalIndex+2] / 255.f - inData[inIndex+2] / 255.f);
      }
  }

  cout << "Normalized error: " << error << endl;

  free(inData);
  free(originalData);

  // the last argument is static_data, if it is false, after this call the
  // pointer to the data is owned by the wxImage object, which will be
  // responsible for deleting it. So this means that you should not delete the
  // data yourself.
  inImage.SetData(mergedData, WIDTH * 2, HEIGHT, false);

  // Set up the scrolled window as a child of this frame
  scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
  scrolledWindow->SetScrollbars(10, 10, WIDTH * 2, HEIGHT);
  scrolledWindow->SetVirtualSize(WIDTH * 2, HEIGHT);

  // Bind the paint event to the OnPaint function of the scrolled window
  scrolledWindow->Bind(wxEVT_PAINT, &MyFrame::OnPaint, this);

  // Set the frame size
  SetClientSize(WIDTH * 2, HEIGHT);

  // Set the frame background color
  SetBackgroundColour(*wxBLACK);
}

/**
 * The OnPaint handler that paints the UI.
 * Here we paint the image pixels into the scrollable window.
 */
void MyFrame::OnPaint(wxPaintEvent &event) {
  wxBufferedPaintDC dc(scrolledWindow);
  scrolledWindow->DoPrepareDC(dc);

  wxBitmap inImageBitmap = wxBitmap(inImage);
  dc.DrawBitmap(inImageBitmap, 0, 0, false);
}

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height) {

  // Open the file in binary mode
  ifstream inputFile(imagePath, ios::binary);

  if (!inputFile.is_open()) {
    cerr << "Error Opening File for Reading" << endl;
    exit(1);
  }

  // Create and populate RGB buffers
  vector<char> Rbuf(width * height);
  vector<char> Gbuf(width * height);
  vector<char> Bbuf(width * height);

  /**
   * The input RGB file is formatted as RRRR.....GGGG....BBBB.
   * i.e the R values of all the pixels followed by the G values
   * of all the pixels followed by the B values of all pixels.
   * Hence we read the data in that order.
   */

  inputFile.read(Rbuf.data(), width * height);
  inputFile.read(Gbuf.data(), width * height);
  inputFile.read(Bbuf.data(), width * height);

  inputFile.close();

  /**
   * Allocate a buffer to store the pixel values
   * The data must be allocated with malloc(), NOT with operator new. wxWidgets
   * library requires this.
   */
  unsigned char *inData =
      (unsigned char *)malloc(width * height * 3 * sizeof(unsigned char));
      
  for (int i = 0; i < height * width; i++) {
    // We populate RGB values of each pixel in that order
    // RGB.RGB.RGB and so on for all pixels
    inData[3 * i] = Rbuf[i];
    inData[3 * i + 1] = Gbuf[i];
    inData[3 * i + 2] = Bbuf[i];
  }

  return inData;
}

unsigned char*** convertToBlocks(unsigned char* imageData, int channel) {
  // Create a 3D array of 8x8 blocks
  unsigned char ***blocks = (unsigned char ***)malloc(WIDTH * HEIGHT / 64 * sizeof(unsigned char**));
  for (int block = 0; block < WIDTH * HEIGHT / 64; block++) {
    blocks[block] = (unsigned char**)malloc(8 * sizeof(unsigned char*));
    for (int row = 0; row < 8; row++) {
      blocks[block][row] = (unsigned char*)malloc(8 * sizeof(unsigned char));
    }
  }

  int numBlocksX = WIDTH / 8;

  for (int row = 0; row < HEIGHT; row++) {
    for (int col = 0; col < WIDTH; col++) {
      int blockX = col / 8;
      int blockY = row / 8;

      int x = col % 8;
      int y = row % 8;
      int block = numBlocksX * blockY + blockX;

      blocks[block][y][x] = imageData[(row * WIDTH + col) * 3 + channel];
    }
  }

  return blocks;
}

unsigned char* convertToImageData(unsigned char*** rBlocks, unsigned char*** gBlocks, unsigned char*** bBlocks) {
  unsigned char *imageData = (unsigned char *)malloc(WIDTH * HEIGHT * 3 * sizeof(unsigned char));

  int numBlocksX = WIDTH / 8;
  int numBlocksY = HEIGHT / 8;

  for (int blockY = 0; blockY < numBlocksY; blockY++) {
    for (int blockX = 0; blockX < numBlocksX; blockX++) {
      int block = numBlocksX * blockY + blockX;

      for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
          int col = blockX * 8 + x;
          int row = blockY * 8 + y;

          imageData[(row * WIDTH + col) * 3] = rBlocks[block][y][x];
          imageData[(row * WIDTH + col) * 3 + 1] = gBlocks[block][y][x];
          imageData[(row * WIDTH + col) * 3 + 2] = bBlocks[block][y][x];
        }
      }
    }
  }

  return imageData;
}

void free3DArray(unsigned char*** blocks) {
  for (int block = 0; block < WIDTH * HEIGHT / 64; block++) {
    for (int row = 0; row < 8; row++) {
      free(blocks[block][row]);
    }
    free(blocks[block]);
  }
  free(blocks);
}

wxIMPLEMENT_APP(MyApp);