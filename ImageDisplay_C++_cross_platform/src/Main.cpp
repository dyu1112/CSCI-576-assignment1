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
double DCT(int u, int v, double** f);
double IDCT(int x, int y, double** F);
double C(int k);

double*** convertToBlocks(unsigned char* imageData, int channel);
unsigned char* convertToImageData(double*** rBlocks, double*** gBlocks, double*** bBlocks);
double*** create3DArray();
void free3DArray(double*** blocks);


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

  cout << "Original first pixel R channel = " << (float)(inData[0]) << endl;

  double*** rBlocksf = convertToBlocks(inData, 0);
  double*** gBlocksf = convertToBlocks(inData, 1);
  double*** bBlocksf = convertToBlocks(inData, 2);
  free(inData);
  double*** rBlocksF = create3DArray();
  double*** gBlocksF = create3DArray();
  double*** bBlocksF = create3DArray();

  cout << "Finished Blocking. First block: " << endl;
  for (int u = 0; u < 8; u++) {
    for (int v = 0; v < 8; v++) {
      cout << rBlocksf[0][u][v] << endl;
    }
  }

  // Sequential Mode
  if (M = 1) {
    // Encode
    for (int block = 0; block < WIDTH * HEIGHT / 64; block++) {
      for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {
          rBlocksF[block][u][v] = round( DCT(u,v, rBlocksf[block]) / powf(2.0f,N) );
          gBlocksF[block][u][v] = round( DCT(u,v, gBlocksf[block]) / powf(2.0f,N) );
          bBlocksF[block][u][v] = round( DCT(u,v, bBlocksf[block]) / powf(2.0f,N) );
        }
      }
    }

    cout << "Finished Encoding. rBlocksF[0][0][0] = " << rBlocksF[0][0][0] << endl;

    // Dequantize
    for (int block = 0; block < WIDTH * HEIGHT / 64; block++) {
      for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {
          // Dequantize
          rBlocksF[block][u][v] *= pow(2.0f, N);
          gBlocksF[block][u][v] *= pow(2.0f, N);
          bBlocksF[block][u][v] *= pow(2.0f, N);
        }
      }
    }

    cout << "Finished Dequantizing. rBlocksF[0][0][0] = " << rBlocksF[0][0][0] << endl;

    // Decode
    for (int block = 0; block < WIDTH * HEIGHT / 64; block++) {
      for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
          rBlocksf[block][x][y] = IDCT(x,y,rBlocksF[block]);
          gBlocksf[block][x][y] = IDCT(x,y,gBlocksF[block]);
          bBlocksf[block][x][y] = IDCT(x,y,bBlocksF[block]);
        }
      }
    }
  }
  
  cout << "Finished Decoding. rBlocksf[0][0][0] = " << rBlocksf[0][0][0] << endl;
  
  free3DArray(rBlocksF);
  free3DArray(gBlocksF);
  free3DArray(bBlocksF);

  unsigned char *outData = convertToImageData(rBlocksf,gBlocksf,bBlocksf);
  
  free3DArray(rBlocksf);
  free3DArray(gBlocksf);
  free3DArray(bBlocksf);

  //cout << "After processing: " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 1]) << ", " << static_cast<int>(inData[3 * DEBUG_X * DEBUG_Y + 2]) << endl; 
  cout << "Displaying image..." << endl;
  MyFrame *frame = new MyFrame("Image Display", imagePath, outData);
  frame->Show(true);

  // return true to continue, false to exit the application
  return true;
}

double C(int k) 
{
  if (k == 0) {
    return 1.0 / sqrt(2.0);
  }
  else {
    return 1.0;
  }
}

double DCT(int u, int v, double** f) 
{
  double result = (1.0/4.0) * C(u) * C(v);
  double sum = 0.0;
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      sum += f[x][y] * cos(((2.0*x + 1.0) * u * M_PI) / 16.0) * cos(((2.0*y + 1.0) * v * M_PI) / 16.0);
    }
  }

  return result * sum;
}

double IDCT(int x, int y, double** F)
{
  double result = 1.0 / 4.0;
  double sum = 0.0;
  for (int u = 0; u < 8; u++) {
    for (int v = 0; v < 8; v++) {
      sum += C(u) * C(v) * F[u][v] * cos(((2.0*x + 1.0) * u * M_PI) / 16.0) * cos(((2.0*y + 1.0) * v * M_PI) / 16.0);
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
      }
  }

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

double*** convertToBlocks(unsigned char* imageData, int channel) {
  // Create a 3D array of 8x8 blocks
  double ***blocks = create3DArray();

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

unsigned char* convertToImageData(double*** rBlocks, double*** gBlocks, double*** bBlocks) {
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

          imageData[(row * WIDTH + col) * 3] = clamp<double>(rBlocks[block][y][x], 0, 255);
          imageData[(row * WIDTH + col) * 3 + 1] = clamp<double>(gBlocks[block][y][x], 0, 255);
          imageData[(row * WIDTH + col) * 3 + 2] = clamp<double>(bBlocks[block][y][x], 0, 255);
        }
      }
    }
  }

  return imageData;
}

double*** create3DArray() {
  // Create a 3D array of 8x8 blocks
  double ***blocks = (double ***)malloc(WIDTH * HEIGHT / 64 * sizeof(double**));
  for (int block = 0; block < WIDTH * HEIGHT / 64; block++) {
    blocks[block] = (double**)malloc(8 * sizeof(double*));
    for (int row = 0; row < 8; row++) {
      blocks[block][row] = (double*)malloc(8 * sizeof(double));
    }
  }

  return blocks;
}

void free3DArray(double*** blocks) {
  for (int block = 0; block < WIDTH * HEIGHT / 64; block++) {
    for (int row = 0; row < 8; row++) {
      free(blocks[block][row]);
    }
    free(blocks[block]);
  }
  free(blocks);
}

wxIMPLEMENT_APP(MyApp);