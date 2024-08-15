#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>

#define NUM_OF_THREADS 3
#define RED 0
#define GREEN 1
#define BLUE 2

using namespace std;

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#pragma pack(push, 1)
typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

struct {
    char* fileBuffer;
    int bufferSize;
    vector<vector<unsigned char>> redChannel;
    vector<vector<unsigned char>> greenChannel;
    vector<vector<unsigned char>> blueChannel;
} Photo;

struct {
    vector<vector<unsigned char>> red;
    vector<vector<unsigned char>> green;
    vector<vector<unsigned char>> blue;
} tempChannel;

#pragma pack(pop)

//Declaring a filter for furthur use
int gaussianBlur[3][3] = {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}};
float gaussianBlurCoef = 0.0625;

int rows;
int cols;
long fileLength; //total length of the bmp file



bool fillAndAllocate(char*& buffer, const char* fileName, int& rows, int& cols, int& bufferSize) {
    std::ifstream file(fileName);
    if (!file) {
        std::cout << "File" << fileName << " doesn't exist!" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);
    fileLength = length;
    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return true;
}

void allocChannels(){
    Photo.redChannel.resize(rows, vector<unsigned char>(cols));
    Photo.greenChannel.resize(rows, vector<unsigned char>(cols));
    Photo.blueChannel.resize(rows, vector<unsigned char>(cols));
}

void getPixelsFromBMP24(int end, int rows, int cols, char* fileReadBuffer, int channel) {
    int count = (channel == 0) ? 1 : (channel == 1) ? 2 : 3;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++) {
        count += extra;
        for (int j = cols - 1; j >= 0; j--) {
                switch (channel) {
                case RED:
                    // fileReadBuffer[end - count] is the red value
                    Photo.redChannel[i][j] = fileReadBuffer[end - count];
                    break;
                case GREEN:
                    Photo.greenChannel[i][j] = fileReadBuffer[end - count];
                    // fileReadBuffer[end - count] is the green value
                    break;
                case BLUE:
                    Photo.blueChannel[i][j] = fileReadBuffer[end - count];
                    // fileReadBuffer[end - count] is the blue value
                    break;
                }
                count += 3;
                // go to the next position in the buffer
        }
    }
}

void* parallelRead(void* arg){
    int channel_id = *((int*)arg);
    getPixelsFromBMP24(fileLength, rows, cols, Photo.fileBuffer, channel_id);
    pthread_exit(NULL);
}

void writeOutBmp24(char* fileBuffer, const char* nameOfFileToCreate, int bufferSize) {
    std::ofstream write(nameOfFileToCreate);
    if (!write) {
        std::cout << "Failed to write " << nameOfFileToCreate << std::endl;
        return;
    }

    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++) {
        count += extra;
        for (int j = cols - 1; j >= 0; j--) {
            for (int k = 0; k < 3; k++) {
                switch (k) {
                case 0:
                    fileBuffer[bufferSize - count] = Photo.redChannel[i][j];
                    // write red value in fileBuffer[bufferSize - count]
                    break;
                case 1:
                    fileBuffer[bufferSize - count] = Photo.greenChannel[i][j];
                    // write green value in fileBuffer[bufferSize - count]
                    break;
                case 2:
                    fileBuffer[bufferSize - count] = Photo.blueChannel[i][j];
                    // write blue value in fileBuffer[bufferSize - count]
                    break;
                }
                count++;
                // go to the next position in the buffer
            }
        }
    }
    write.write(fileBuffer, bufferSize);
}

void verticalMirror(int rows, int cols, int channel){
    unsigned char temp;
    for (int i = 0; i < (rows/2) ; i++) {
        for (int j = 0; j < cols; j++) {   
            switch (channel) {
                case RED:
                    temp = Photo.redChannel[i][j];
                    Photo.redChannel[i][j] = Photo.redChannel[rows - i - 1][j];
                    Photo.redChannel[rows - i - 1][j] = temp;
                    break;
                case GREEN:
                    temp = Photo.greenChannel[i][j];
                    Photo.greenChannel[i][j] = Photo.greenChannel[rows - i - 1][j];
                    Photo.greenChannel[rows - i - 1][j] = temp;
                    break;
                case BLUE:
                    temp = Photo.blueChannel[i][j];
                    Photo.blueChannel[i][j] = Photo.blueChannel[rows - i - 1][j];
                    Photo.blueChannel[rows - i - 1][j] = temp;
                    break;
            }
        }
    }
}

void* parallelFlip(void* arg){ //Flips the image, then applies the kernel
    int channel_id = *((int*)arg);
    verticalMirror(rows, cols, channel_id);
    pthread_exit(NULL);
}

void edgeHandler(int rows, int cols){
    for(int i = 0; i < rows; i++){
        if(i == 0){
            Photo.redChannel[i] = Photo.redChannel[i+1];
            Photo.greenChannel[i] = Photo.greenChannel[i+1];
            Photo.blueChannel[i] = Photo.blueChannel[i+1];
        }
        else if(i == rows-1){
            Photo.redChannel[i] = Photo.redChannel[i-1];
            Photo.greenChannel[i] = Photo.greenChannel[i-1];
            Photo.blueChannel[i] = Photo.blueChannel[i-1];
        }
        else {
            Photo.redChannel[i][0] = Photo.redChannel[i][1];
            Photo.redChannel[i][cols-1] = Photo.redChannel[i][cols-2];
            Photo.greenChannel[i][0] = Photo.greenChannel[i][1];
            Photo.greenChannel[i][cols-1] = Photo.greenChannel[i][cols-2];
            Photo.blueChannel[i][0] = Photo.blueChannel[i][1];
            Photo.blueChannel[i][cols-1] = Photo.blueChannel[i][cols-2];
        }
    }
}

//function written with a 3*3 kernel in mind
void applyKernel(int rows, int cols, int kernel[3][3], float norm, int channel){
    
    //make a copy of current channels
    vector<vector<unsigned char>> tempChannel;
    switch(channel){
        case RED:
            tempChannel = Photo.redChannel;
        break;
        case GREEN:
            tempChannel = Photo.greenChannel;
        break;
        case BLUE:
            tempChannel = Photo.blueChannel;
        break;
    }

    for(int i = 1; i < rows - 1; i++){
        for(int j = 1; j < cols - 1; j++){
            float sum = 0;

            for(int k = -1; k <= 1; k++){
                for(int l = -1; l <= 1; l++){
                    sum += tempChannel[i + k][j + l] * kernel[k + 1][l + 1] * norm;
                }
            }

            //Check if the output of filter is still in bounderies of our pixels colors
            sum = (sum < 0) ? 0 : (sum > 255) ? 255 : (sum);
            
            switch(channel){
                case RED:
                    Photo.redChannel[i][j] = static_cast<unsigned char>(sum);
                break;
                case GREEN:
                    Photo.greenChannel[i][j] = static_cast<unsigned char>(sum);
                break;
                case BLUE:
                    Photo.blueChannel[i][j] = static_cast<unsigned char>(sum);
                break;
            }
        }
    }
    
}

void* parallelKernel(void* arg){
    int channel_id = *((int*)arg);
    applyKernel(rows, cols, gaussianBlur, gaussianBlurCoef, channel_id);
    pthread_exit(NULL);
}

void purpleHaze(int rows, int cols, int channel){
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            switch(channel){
                case RED:
                    Photo.redChannel[i][j] = min(255.0 , (0.5 * tempChannel.red[i][j]) + (0.3 * tempChannel.green[i][j]) + (0.5 * tempChannel.blue[i][j]));
                    break;
                case GREEN:
                    Photo.greenChannel[i][j] = min(255.0, (0.16 * tempChannel.red[i][j]) + (0.5 * tempChannel.green[i][j]) + (0.16 * tempChannel.blue[i][j]));
                    break;
                case BLUE:
                    Photo.blueChannel[i][j] = min(255.0 ,(0.6 * tempChannel.red[i][j]) + (0.2 * tempChannel.green[i][j]) + (0.8 * tempChannel.blue[i][j]));
                    break;
            }
        }
    }
}

void* parallelPuprleHaze(void* arg){
    int channel_id = *((int*)arg);
    purpleHaze(rows, cols, channel_id);
    pthread_exit(NULL);
}

void drawDiagonalLines(int rows, int cols, int channel){
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            if((i+j == rows) || (i+j == rows/2) || (i+j == rows + rows/2)){
                // white color code -> 255
                switch(channel){
                    case RED:
                        Photo.redChannel[i][j] = 255;
                    break;
                    case GREEN:
                        Photo.greenChannel[i][j] = 255;
                    break;
                    case BLUE:
                        Photo.blueChannel[i][j] = 255;
                    break;
                    }
            }
        }
    }
}

void* parallelDraw(void* arg){
    int channel_id = *((int*)arg);
    drawDiagonalLines(rows, cols, channel_id);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    pthread_t threads[NUM_OF_THREADS];
    vector<int> channels = {RED, GREEN, BLUE};

    auto exec_start = chrono::high_resolution_clock::now();

    if (!fillAndAllocate(Photo.fileBuffer, argv[1], rows, cols, Photo.bufferSize)) {
        std::cout << "File read error" << std::endl;
        return 1;
    }

    auto fp_time = chrono::high_resolution_clock::now();
    
    //Store each channel seperately
    allocChannels();

    // read input file concurrently with 3 threads (one thread for each color channel)
    for(int i = 0; i < NUM_OF_THREADS; i++){
        channels[i] = i;
        int thread_status = pthread_create(&threads[i], NULL, parallelRead, (void*)&channels[i]);
        if(thread_status){
            cerr << "Error: Unable to create thread " << endl;
            return 1;
        }
    }
    for(int i = 0; i < NUM_OF_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    auto read_end = chrono::high_resolution_clock::now();

    // flip the image
    for(int i = 0; i < NUM_OF_THREADS; i++){
        channels[i] = i;
        int thread_status = pthread_create(&threads[i], NULL, parallelFlip, (void*)&channels[i]);
        if(thread_status){
            cerr << "Error: Unable to create thread " << endl;
            return 1;
        }
    }
    for(int i = 0; i < NUM_OF_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    auto flip_end = chrono::high_resolution_clock::now();

    // apply chosen Kernel
    for(int i = 0; i < NUM_OF_THREADS; i++){
        channels[i] = i;
        int thread_status = pthread_create(&threads[i], NULL, parallelKernel, (void*)&channels[i]);
        if(thread_status){
            cerr << "Error: Unable to create thread " << endl;
            return 1;
        }
    }
    for(int i = 0; i < NUM_OF_THREADS; i++){
        pthread_join(threads[i], NULL);
    }
    edgeHandler(rows, cols);

    auto kernel_end = chrono::high_resolution_clock::now();

    // copy current state of channels
    for(int i = 0; i < 3; i++){
        tempChannel.red = Photo.redChannel;
        tempChannel.green = Photo.greenChannel;
        tempChannel.blue = Photo.blueChannel;
    }

    // apply purple haze effect
    for(int i = 0; i < NUM_OF_THREADS; i++){
        channels[i] = i;
        int thread_status = pthread_create(&threads[i], NULL, parallelPuprleHaze, (void*)&channels[i]);
        if(thread_status){
            cerr << "Error: Unable to create thread " << endl;
            return 1;
        }
    }
    for(int i = 0; i < NUM_OF_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    auto haze_end = chrono::high_resolution_clock::now();

    // draw diagonal lines 
    for(int i = 0; i < NUM_OF_THREADS; i++){
        channels[i] = i;
        int thread_status = pthread_create(&threads[i], NULL, parallelDraw, (void*)&channels[i]);
        if(thread_status){
            cerr << "Error: Unable to create thread " << endl;
            return 1;
        }
    }
    for(int i = 0; i < NUM_OF_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    auto draw_end = chrono::high_resolution_clock::now();

    // write output file (No parallelism in this part)
    writeOutBmp24(Photo.fileBuffer, "output.bmp", Photo.bufferSize);

    auto write_end = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> read_time = read_end - fp_time;
    chrono::duration<double, milli> flip_time = flip_end - read_end;
    chrono::duration<double, milli> kernel_time = kernel_end - flip_end;
    chrono::duration<double, milli> haze_time = haze_end - kernel_end;
    chrono::duration<double, milli> draw_time = draw_end - haze_end;
    chrono::duration<double, milli> write_time = write_end - draw_end;
    chrono::duration<double, milli> total_time = write_end - exec_start;

    cout << "Read : " << read_time.count() << " ms" << endl << "Flip : " << flip_time.count() << " ms" << endl
        << "Blur : " << kernel_time.count() << " ms" << endl << "Purple : " << haze_time.count() << " ms" << endl
        << "Lines : " << draw_time.count() << " ms" << endl << "Execution : " << total_time.count() << " ms" << endl;

    return 0;
}