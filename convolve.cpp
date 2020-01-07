// necessary libraries

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <ctime>

#define SWAP(a,b)  tempr=(a);(a)=(b);(b)=tempr

using namespace std; // bad practice in professional applications but it shouldnt cause issue in our case.

/* declare variables to store the parts of a wav file */

// RIFF chunk descriptor
char chunkID[4];
int chunkSize;
char format[2];

// fmt sub-chunk
char subChunk1ID[4];
int subChunk1Size;
int16_t audioFormat;
int16_t numChannels;
int sampleRate;
int byteRate;
int16_t blockAlign;
int16_t bitsPerSample;

// data subchunk
char subChunk2ID[4];
int subChunk2Size;
short* fileData;

int sampleRateSource, sampleRateImpulse; // for later discrepancy checking as this program needs identical input file sample rates to function
int size;

string requestFileName(string fileNameIn);

float *readWav(string filename, float *signal);
void writeWav(string fileName, float *signal, int signalSize);

void convolutionProcess(float *x, int inputSignalSize, float *h, int responseSignalSize, float *y, int outputSignalSize);

int main()
{

    string sourceFileName, responseFileName, outputFileName;

    sourceFileName = requestFileName("input");
    responseFileName = requestFileName("response");
    outputFileName = requestFileName("output");

    /*
    if (responseFileName == "same") // can't just set it as nothing while using cin unfortunately
    responseFileName = sourceFileName;
    */

    int inputSignalSize, responseSignalSize, outputSignalSize;

    float *x, *h, *y; // array declaration. the readwav functions below fill them with the corresponding samples for the above's lengths

    cout << endl; // read source
    x = readWav(sourceFileName, x);
    inputSignalSize = size;
    sampleRateSource = sampleRate;

    cout << endl; // read impulse
    h = readWav(responseFileName, h);
    responseSignalSize = size;
    sampleRateImpulse = sampleRate;

    if (sampleRateSource == sampleRateImpulse) { // sample rate issue checking
        cout << "Sample rates are identical: " << (double)sampleRateSource / 1000 << " kHz." << endl << endl;
    } else {
        cout << "ERROR: Sample rates differ. " << (double)sampleRateSource / 1000 << " kHz on " << sourceFileName << " and " << (double)sampleRateImpulse / 1000 << " kHz on " << responseFileName << ". Re-check and try again." << endl << endl;
        return 0;
    }

    outputSignalSize = inputSignalSize + responseSignalSize - 1;
    y = new float[outputSignalSize];
    clock_t begin = clock();
    cout << "Starting Process / Clock." << endl;

    convolutionProcess(x, inputSignalSize, h, responseSignalSize, y, outputSignalSize);

    writeWav(outputFileName, y, outputSignalSize);

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    cout << "Done. Convolution finished in " << elapsed_secs << " seconds." << endl;

    return 0;
}

// reads in a wav file into an array
float *readWav (string filename, float *signal)
{

	ifstream inputfile(filename.c_str(), ios::in | ios::binary);

	inputfile.seekg(ios::beg);

	inputfile.read(chunkID, 4);
	inputfile.read((char*) &chunkSize, 4);
	inputfile.read(format, 4);
	inputfile.read(subChunk1ID, 4);
	inputfile.read((char*) &subChunk1Size, 4);
	inputfile.read((char*) &audioFormat, 2);
	inputfile.read((char*) &numChannels, 2);
	inputfile.read((char*) &sampleRate, 4);
	inputfile.read((char*) &byteRate, 4);
	inputfile.read((char*) &blockAlign, 2);
	inputfile.read((char*) &bitsPerSample, 2);
	if (subChunk1Size == 18) {
		inputfile.seekg(2, ios::cur);
	}
	inputfile.read(subChunk2ID, 4);
	inputfile.read((char*)&subChunk2Size, 4);
	size = subChunk2Size / 2;

	short *data = new short[size];
	for (int i = 0; i < size; i++) {
		inputfile.read((char *) &data[i], 2);
	}

	inputfile.close();

	short sample;
	signal = new float[size];

	cout << "File " << filename << " is " << size << " samples in length." << endl;

	for (int i = 0; i < size; i++) {
		sample = data[i];
		signal[i] = (sample * 1.0) / (32767);
		if (signal[i] < -1.0)
			signal[i] = -1.0;
	}


	return signal;
}

// writes a signal to a wav file
void writeWav(string filename, float *signal, int signalSize)
{

	ofstream outputfile(filename.c_str(), ios::out | ios::binary);
	// PCM = 18 was unnecessary
	subChunk1Size = 16;

	subChunk2Size = numChannels * signalSize * (bitsPerSample / 8);
	chunkSize = subChunk2Size + 36;
    // full disclosure: i have no idea how this really works...
    //i kind of just followed someone's instructions on how to write this.
	outputfile.write(chunkID, 4);
	outputfile.write((char*) &chunkSize, 4);
	outputfile.write(format, 4);
	outputfile.write(subChunk1ID, 4);
	outputfile.write((char*) &subChunk1Size, 4);
	outputfile.write((char*) &audioFormat, 2);
	outputfile.write((char*) &numChannels, 2);
	outputfile.write((char*) &sampleRate, 4);
	outputfile.write((char*) &byteRate, 4);
	outputfile.write((char*) &blockAlign, 2);
	outputfile.write((char*) &bitsPerSample, 2);
	outputfile.write(subChunk2ID, 4);
	outputfile.write((char*)&subChunk2Size, 4);

	int16_t sample;

	// converting float to int between -2^15 to 2^15 - 1
	for(int i = 0; i < signalSize; i++)
	{
		sample = (int16_t)(signal[i] * (32767));
		outputfile.write((char*)&sample, 2);
	}
	outputfile.close();
}

// four1 from numerical recepies in C. unchanged aside from different variable types (floats rather than doubles).
void four1(float data[], int nn, int isign)
{
    unsigned long n, mmax, m, j, istep, i;
    float wtemp, wr, wpr, wpi, wi, theta;
    float tempr, tempi;

    n = nn << 1;
    j = 1;

    for (i = 1; i < n; i += 2) {
        if (j > i) {
            SWAP(data[j], data[i]);
            SWAP(data[j+1], data[i+1]);
        }
        m = nn;
        while (m >= 2 && j > m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    mmax = 2;
    while (n > mmax) {
        istep = mmax << 1;
        theta = isign * (6.28318530717959 / mmax);
        wtemp = sin(0.5 * theta);
        wpr = -2.0 * wtemp * wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m = 1; m < mmax; m += 2) {
            for (i = m; i <= n; i += istep) {
                j = i + mmax;
                tempr = wr * data[j] - wi * data[j+1];
                tempi = wr * data[j+1] + wi * data[j];
                data[j] = data[i] - tempr;
                data[j+1] = data[i+1] - tempi;
                data[i] += tempr;
                data[i+1] += tempi;
            }
            wr = (wtemp = wr) * wpr - wi * wpi + wr;
            wi = wi * wpr + wtemp * wpi + wi;
        }
        mmax = istep;
    }
}

// scales the numbers in a given array signal[] and stores numbers back in array
void four1Scaling (float signal[], int inputSignalSize)
{
	int k;
	int i;
	for (k = 0, i = 0; k < inputSignalSize; k++, i+=2) { // runs through the loop inputSignalSize number of times
		signal[i] /= (float)inputSignalSize;
		signal[i+1] /= (float)inputSignalSize;
	}
}

void complexCalculation(float complexInput[],float complexIR[],float complexResult[], int size)
{
	int i = 0;
	int tempI = 0;
	for(i = 0; i < size; i++) {
		tempI = i * 2;
	    complexResult[tempI] = complexInput[tempI] * complexIR[tempI] - complexInput[tempI+1] * complexIR[tempI+1];
	    complexResult[tempI+1] = complexInput[tempI+1] * complexIR[tempI] + complexInput[tempI] * complexIR[tempI+1];
	}
}

void padZeroes(float toPad[], int size)
{
	memset(toPad, 0, size);
}

void unpadArray(float result[], float complete[], int size)
{
	int i, j;

    for(i = 0, j = 0; i < size; i++, j+=2)
    {
	    complete[i] = result[j];
    }
}

void padArray(float output[],float data[], int dataLen, int size)
{
	int i, k;
	for(i = 0, k = 0; i < dataLen; i++, k+=2)
	{
	    output[k] = data[i];
	    output[k + 1] = 0;
	}
	i = k;

	memset(output + k, 0, size -1);
}

void scaleSignal(float signal[], int samples)
{
	float min = 0, max = 0;
	int i = 0;

	for(i = 0; i < samples; i++)
	{
		if(signal[i] > max)
			max = signal[i];
		if(signal[i] < min)
			min = signal[i];
	}

	min = min * -1;
	if(min > max)
		max = min;

	for(i = 0; i < samples; i++)
	{
		signal[i] = signal[i] / max;
	}
}

void convolutionProcess(float *x,int inputSignalSize,float * h,int responseSignalSize, float *y, int outputSignalSize)
{
	int totalSize = 0;
	int paddedTotalSize = 1;
	totalSize = inputSignalSize + responseSignalSize - 1;

	int i = 0;
	while (paddedTotalSize < totalSize)
	{
		paddedTotalSize <<= 1;
		i++;
	}

	float *complexResult = new float[2*paddedTotalSize];
	float *input = new float[2*paddedTotalSize];
	float *ir = new float[2*paddedTotalSize];

    //the easy stuff
	padArray(input,x, inputSignalSize,2*paddedTotalSize);
	padArray(ir,h, responseSignalSize, 2*paddedTotalSize);
	padZeroes(complexResult, 2*paddedTotalSize);

	//the hard stuff
	four1(input-1, paddedTotalSize, 1);
	four1(ir-1, paddedTotalSize, 1);

	complexCalculation(input, ir, complexResult, paddedTotalSize);

	four1(complexResult-1, paddedTotalSize, -1);
	four1Scaling(complexResult, paddedTotalSize);

	unpadArray(complexResult, y, outputSignalSize);
	scaleSignal(y, outputSignalSize);
}

string requestFileName(string fileNameIn)
{
    cout << "Please enter filename for the "<<fileNameIn<<" target file." << endl;
    string retValue = "";
    cin >> retValue;

    // check for valid input
     if(retValue == "" || retValue.length() < 1)
    {
            cout << "You didn't enter in a valid file name!" << endl;
            return 0;
    }

    //add correct file extension
    if(retValue.length() < 4)
            retValue = retValue + ".wav";
    else if(retValue.substr(retValue.length() - 4,4) != ".wav")
            retValue = retValue + ".wav";

    return retValue;
}
