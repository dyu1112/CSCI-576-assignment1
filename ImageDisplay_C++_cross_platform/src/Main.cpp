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
const int WIDTH = 512;
const int HEIGHT = 512;
const int DEBUG_X = 508;
const int DEBUG_Y = 508;

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
  MyFrame(const wxString &title, unsigned char *inData);

 private:
  void OnPaint(wxPaintEvent &event);
  wxImage inImage;
  wxScrolledWindow *scrolledWindow;
};

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height);

/** Utility function to read image data */
float *normalizeImageData(unsigned char *inData, int width, int height);

void uniformQuantization(unsigned char* inData, int q1, int q2, int q3);
void uniformQuantization(float* inData, int q1, int q2, int q3);

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
  if (wxApp::argc != 7) {
    cerr << "The executable should be invoked with exactly 6 arguments: "
            "YourProgram.exe C:/myDir/myImage.rgb C M Q1 Q2 Q3"
         << endl;
    exit(1);
  }
  cout << "First argument: " << wxApp::argv[0] << endl;
  cout << "Second argument: " << wxApp::argv[1] << endl;
  cout << "Third argument: " << wxApp::argv[2] << endl;
  cout << "Fourth argument: " << wxApp::argv[3] << endl;
  cout << "Fifth argument: " << wxApp::argv[4] << endl;
  cout << "Sixth argument: " << wxApp::argv[5] << endl;
  cout << "Seventh argument: " << wxApp::argv[6] << endl;
  string imagePath = wxApp::argv[1].ToStdString();
  int colorMode = stoi(wxApp::argv[2].ToStdString());
  int quantizationMode = stoi(wxApp::argv[3].ToStdString());
  int q1 = stoi(wxApp::argv[4].ToStdString());
  int q2 = stoi(wxApp::argv[5].ToStdString());
  int q3 = stoi(wxApp::argv[6].ToStdString());

  //MyFrame *frame = new MyFrame("Image Display", imagePath);
  //frame->Show(true);

  unsigned char *inData = readImageData(imagePath, WIDTH, HEIGHT);
  cout << "Original: " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 1]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 2]) << endl; 

  // C
  if (colorMode == 1) {
    if (quantizationMode == 1) {
      uniformQuantization(inData, q1, q2, q3);
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
      cout << "In YUV after uniform quantization: " << data[3 * DEBUG_X * DEBUG_Y] << ", " << data[3 * DEBUG_X * DEBUG_Y + 1] << ", " << data[3 * DEBUG_X * DEBUG_Y + 2] << endl; 
    }

    // YUV to RGB
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
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

  cout << "After processing: " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 1]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 2]) << endl; 
  MyFrame *frame = new MyFrame("Image Display", inData);
  frame->Show(true);

  // return true to continue, false to exit the application
  return true;
}

void uniformQuantization(unsigned char* inData, int q1, int q2, int q3) {
  int regions1 = pow(2,q1);
  int regions2 = pow(2,q2);
  int regions3 = pow(2,q3);

  unsigned char *upperBounds1 =
      (unsigned char *)malloc((regions1 + 1) * sizeof(unsigned char));
  unsigned char *upperBounds2 =
      (unsigned char *)malloc((regions2 + 1) * sizeof(unsigned char));
  unsigned char *upperBounds3 =
      (unsigned char *)malloc((regions3 + 1) * sizeof(unsigned char));

  // Create regions
  for (int i = 1; i <= regions1; i++) {
    upperBounds1[i] = 255.f / regions1 * i;
  }
  upperBounds1[0] = 0;
  upperBounds1[regions1] = 255;

  for (int i = 1; i <= regions2; i++) {
    upperBounds2[i] = 255.f / regions2 * i;
  }
  upperBounds2[0] = 0;
  upperBounds2[regions2] = 255;

  for (int i = 1; i <= regions3; i++) {
    upperBounds3[i] = 255.f / regions3 * i;
  }
  upperBounds3[0] = 0;
  upperBounds3[regions3] = 255;

  // Quantize
  for (int i = 0; i < HEIGHT * WIDTH; i++) {
    // Channel 1
    for (int j = 1; j <= regions1; j++) {
      if (inData[3*i] <= upperBounds1[j]) {
        inData[3*i] = (upperBounds1[j] + upperBounds1[j-1]) / 2;
        break;
      }
    }
    
    // Channel 2
    for (int j = 1; j <= regions2; j++) {
      if (inData[3*i+1] <= upperBounds2[j]) {
        inData[3*i+1] = (upperBounds2[j] + upperBounds2[j-1]) / 2;
        break;
      }
    }

    // Channel 3
    for (int j = 1; j <= regions3; j++) {
      if (inData[3*i+2] <= upperBounds3[j]) {
        inData[3*i+2] = (upperBounds3[j] + upperBounds3[j-1]) / 2;
        break;
      }
    }
  }

  free(upperBounds1);
  free(upperBounds2);
  free(upperBounds3);
}

void uniformQuantization(float* inData, int q1, int q2, int q3) {
  int regions1 = pow(2,q1);
  int regions2 = pow(2,q2);
  int regions3 = pow(2,q3);

  float *upperBounds1 =
      (float *)malloc((regions1 + 1) * sizeof(float));
  float *upperBounds2 =
      (float *)malloc((regions2 + 1) * sizeof(float));
  float *upperBounds3 =
      (float *)malloc((regions3 + 1) * sizeof(float));

  // Create regions
  for (int i = 1; i <= regions1; i++) {
    upperBounds1[i] = 1.f / regions1 * i;
  }
  upperBounds1[0] = 0.f;
  upperBounds1[regions1] = 1.f;

  for (int i = 0; i <= regions2; i++) {
    upperBounds2[i] = (0.436f * 2) / regions2 * i - 0.436f;
  }
  upperBounds2[regions2] = 0.436f;

  for (int i = 0; i <= regions3; i++) {
    upperBounds3[i] = (0.615f * 2) / regions2 * i - 0.615f;
  }
  upperBounds3[regions3] = 0.615f;

  // Quantize
  for (int i = 0; i < HEIGHT * WIDTH; i++) {
    // Channel 1
    bool inRegion = false;
    for (int j = 1; j <= regions1; j++) {
      if (inData[3*i] <= upperBounds1[j]) {
        inData[3*i] = (upperBounds1[j] + upperBounds1[j-1]) / 2;
        inRegion = true;
        break;
      }
    }
    if (!inRegion) {
      inData[3*i] = (upperBounds1[regions1] + upperBounds1[regions1-1]) / 2;
    }
    
    // Channel 2
    inRegion = false;
    for (int j = 1; j <= regions2; j++) {
      if (inData[3*i+1] <= upperBounds2[j]) {
        inData[3*i+1] = (upperBounds2[j] + upperBounds2[j-1]) / 2;
        inRegion = true;
        break;
      }
    }
    if (!inRegion) {
      inData[3*i+1] = (upperBounds2[regions2] + upperBounds2[regions2-1]) / 2;
    }

    // Channel 3
    inRegion = false;
    for (int j = 1; j <= regions3; j++) {
      if (inData[3*i+2] <= upperBounds3[j]) {
        inData[3*i+2] = (upperBounds3[j] + upperBounds3[j-1]) / 2;
        inRegion = true;
        break;
      }
    }
    if (!inRegion) {
      inData[3*i+2] = (upperBounds3[regions3] + upperBounds3[regions3-1]) / 2;
    }
  }

  free(upperBounds1);
  free(upperBounds2);
  free(upperBounds3);
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

MyFrame::MyFrame(const wxString &title, unsigned char* inData)
    : wxFrame(NULL, wxID_ANY, title) {

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

float *normalizeImageData(unsigned char *inData, int width, int height) {
  /**
   * Allocate a buffer to store the pixel values
   * The data must be allocated with malloc(), NOT with operator new. wxWidgets
   * library requires this.
   */
  float *outData =
      (float *)malloc(width * height * 3 * sizeof(float));
      
  for (int i = 0; i < height * width; i++) {
    // We populate RGB values of each pixel in that order
    // RGB.RGB.RGB and so on for all pixels
    outData[3 * i] = inData[3 * i] / 255.0f;
    outData[3 * i + 1] = inData[3 * i + 1] / 255.0f;
    outData[3 * i + 2] = inData[3 * i + 2] / 255.0f;
  }

  return outData;
}

wxIMPLEMENT_APP(MyApp);