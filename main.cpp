#include <iostream>
#include <fstream>
#include <string.h>
#include <math.h>
#include <stdlib.h>
using namespace std;

// Defines for the mode
#define SPRITE		0
#define PLAYFIELD	1

// A little function for pausing
void Pause();
void LoadFileIntoMem(char * FileName);
void RunScripts();

char FileName[256]; // Stores filename

bool Mode = PLAYFIELD; // Sets graphic mode. 0 = sprite, 1 = playfield

bool Reflected; // Reflected or not

char * FileMem; // File memory

char ASMFileName[256]; // File name to save ASM code in

// BMP class
// Only supports 24 bit bmps for now
class BMP
{
public:
	bool LoadFromMem(); // Loads info from memory
	unsigned long ReadHexLine(int Min, int Max); // Send min and max spots and return total

	void RenderSprite();	// Makes the sprite graphics
	void RenderPlayfield(); // Makes the playfield graphics

private:
	string MagicNum;	// Used to determine OS version
	long Size;			// Size of file
	long DataOffset;	// Offset to data info
	long HeaderBytes;	// Bytes left in header
	long Width, Height;	// Width and height
	int ColorPlanes;	// Number of color planes
	int BitsPerPix;		// Number of bits/pixel
	int Compression;	// Type of compression
	long RawSize;		// Raw size of BMP data
};

unsigned long BMP::ReadHexLine(int Min, int Max)
{
	long Total = 0;
	for (int i=Max; i > Min; --i) // We have to work backwards
	{
		long Temp;
		if (FileMem[i] < 0)
			Temp = 256-abs(FileMem[i]);
		else
			Temp = FileMem[i];
		
		Total += Temp<<(8*(i-(Min+1))); // Adds up the file size via temp
	}
	return Total;
}

bool BMP::LoadFromMem()
{
	// Loads magic number, lame way :D
	MagicNum = FileMem[0]; MagicNum += FileMem[1];

	// Check the magic number
	if (MagicNum != "BM")
	{
		cout << "Error: Invalid BMP file.\n";
		return false;
	}

	Size		= ReadHexLine(0x1, 0x5);	// File size
	DataOffset	= ReadHexLine(0x9, 0xD);	// Data offset
	HeaderBytes = ReadHexLine(0xD, 0x11);	// Bytes left in header
	Width		= ReadHexLine(0x11, 0x15);	// Width
	Height		= ReadHexLine(0x15, 0x19);	// Height
	ColorPlanes = ReadHexLine(0x19, 0x1B);	// Color planes
	BitsPerPix	= ReadHexLine(0x1B, 0x1C);	// Bits/Pix
	Compression = ReadHexLine(0x1D, 0x21);	// Compression
	RawSize		= ReadHexLine(0x21, 0x25);	// Size of raw data

	if (BitsPerPix != 24)
	{
		cout << "Error: Requested BMP format not supported.\nPlease use a 24 bit BMP.\n";
		return false;
	}

	// Makes sure the width is proper
	if (Width != 8 && Mode == SPRITE) // Must be 8 for sprite
	{
		cout << "Error: File must be 8 pixels for compiling a sprite.\nWidth in current BMP: " << Width << " pixels.\n";
		return false;
	}
	else if (Width != 40 && Mode == PLAYFIELD) // Must be 40 for playfield
	{
		cout << "Error: File must be 40 pixels for compiling a playfield.\nWidth in current BMP: " << Width << " pixels.\n";
		return false;
	}

	// Makes sure the height is 192 or less
	if (Height > 192)
	{
		cout << "Error: Height of BMP must be 192 or less.\nCurrent height is: " << Height << " pixels.\n";
		return false;
	}

	// Displays the info
	cout << "\nFile information:\n";
	cout << "Filesize: " << Size << " bytes.\n";
	cout << "Offset to data: " << DataOffset << " bytes.\n";
	cout << "Width: " << Width << " pixels.\n";
	cout << "Height: " << Height << " pixels.\n";
	cout << "Bits per pixel: " << BitsPerPix << ".\n";
	cout << "Color planes: " << ColorPlanes << ".\n";
	cout << "Size of raw data: " << RawSize << " bytes.\n";
	cout << "Bytes per row: " << RawSize/Height;

	// Load was successful, yay!
	return true;
}

void BMP::RenderSprite()
{
	int CountToWidth = 0;
	int SubPixelCount = 0;

	bool ASMPlayer[8]; // Stores the binary line
	int Pixel[3];

	ofstream ASMCodeFile;
	ASMCodeFile.open(ASMFileName);

	// Top part
	ASMCodeFile << ";;; Generated with GeekyLink's BMP to Atari 2600 Graphics compiler\n;;; http://www.gekinzuku.com\n";
	ASMCodeFile << "PlayerGraphic\n";

	// Loops through the data
	for (int i=DataOffset; i < Size+1; ++i)
	{
		unsigned int Temp;
		if (FileMem[i] < 0)
			Temp = 256-abs(FileMem[i]);
		else
			Temp = FileMem[i];

		++CountToWidth;
		// This skips over the 4 byte alignment padding
		if (CountToWidth == Width*3+1)
		{
			ASMCodeFile << "\t.byte #%";
			for (int x=0;x<8;x++)
			{
				ASMCodeFile << ASMPlayer[x];
			}
			ASMCodeFile << "\n";

			i+= (RawSize/Height) - CountToWidth;
			CountToWidth=0;
			continue;
		}

		Pixel[SubPixelCount] = Temp; // Stores the pixel colors for comparing
		++SubPixelCount;
		if (SubPixelCount == 3)
		{
			if (Pixel[0] != 255 || Pixel[1] != 255 || Pixel[2] != 255) // Sees if pixel is not white
			{
			//	cout << "X,Y: " << CountToWidth/3-1 << "," << (i-DataOffset)/(Width*3+2) << ": Pixel not white.\n";
			//	cout << Pixel[0] << "," << Pixel[1] << "," << Pixel[2] << endl;
				ASMPlayer[CountToWidth/3-1] = 1;
			}
			else ASMPlayer[CountToWidth/3-1] = 0;
			//cout << CountToWidth/3 << endl;
			SubPixelCount=0;
			//cout << CountToWidth/3 << endl;
		}
	}
}

void BMP::RenderPlayfield()
{
	int CountToWidth = 0;
	int SubPixelCount = 0;

	bool ASMPlayfield[40][192]; // Stores the binary line
	int Pixel[3];

	// Loops through the data
	for (int i=DataOffset; i < Size+1; ++i)
	{
		unsigned int Temp;
		if (FileMem[i] < 0)
			Temp = 256-abs(FileMem[i]);
		else
			Temp = FileMem[i];

		++CountToWidth;
		// This skips over the 4 byte alignment padding
		if (CountToWidth == Width*3+1)
		{
			i+= (RawSize/Height) - CountToWidth;
			CountToWidth=0;
			continue;
		}

		Pixel[SubPixelCount] = Temp; // Stores the pixel colors for comparing
		++SubPixelCount;
		if (SubPixelCount == 3)
		{
			int Y = (i-DataOffset)/(RawSize/Height);
			if (Pixel[0] != 255 || Pixel[1] != 255 || Pixel[2] != 255) // Sees if pixel is not white
			{
			//	cout << "X,Y: " << CountToWidth/3-1 << "," << (i-DataOffset)/(Width*3+2) << ": Pixel not white.\n";
			//	cout << Pixel[0] << "," << Pixel[1] << "," << Pixel[2] << endl;
				ASMPlayfield[CountToWidth/3-1][Y] = 1;
				//cout << endl << CountToWidth/3-1 << "," << Y << endl;
			}
			else ASMPlayfield[CountToWidth/3-1][Y] = 0;
			//cout << CountToWidth/3 << endl;
			SubPixelCount=0;
			//cout << CountToWidth/3 << endl;
		}
	}

	ofstream ASMCodeFile;
	ASMCodeFile.open(ASMFileName);

	// Top part
	ASMCodeFile << ";;; Generated with GeekyLink's BMP to Atari 2600 Graphics compiler\n;;; http://www.gekinzuku.com\n";

	if (Reflected)
		ASMCodeFile << ";;; Graphic is reflected\n";
	else
		ASMCodeFile << ";;; Graphic is not reflected\n";

	ASMCodeFile << "PF0TitleLeft"; // Playfield 0 left
	for (int y=0; y<Height; ++y)
	{
		ASMCodeFile << "\n\t.byte #%";
		for (int x=3; x>=0; --x) {
			ASMCodeFile << ASMPlayfield[x][y];
		}
		ASMCodeFile << "0000";
	}
	ASMCodeFile << "\nPF0TitleRight";// Playfield 0 Right
	for (int y=0; y<Height; ++y)
	{
		ASMCodeFile << "\n\t.byte #%";

		if (Reflected)
		{
			for (int x=36; x<=39; ++x) {
				ASMCodeFile << ASMPlayfield[x][y];
			}
		}
		else
		{	for (int x=23; x>=20; --x) {
				ASMCodeFile << ASMPlayfield[x][y];
			}
		}
		ASMCodeFile << "0000";
	}

	ASMCodeFile << "\nPF1TitleLeft"; // Playfield 1 left
	for (int y=0; y<Height; ++y)
	{
		ASMCodeFile << "\n\t.byte #%";
		for (int x=4; x<=11; ++x) {
			ASMCodeFile << ASMPlayfield[x][y];
		}
	}
	ASMCodeFile << "\nPF1TitleRight"; // Playfield 1 right
	for (int y=0; y<Height; ++y)
	{
		ASMCodeFile << "\n\t.byte #%";

		if (Reflected) {
			for (int x=35; x>=28; --x) {
				ASMCodeFile << ASMPlayfield[x][y];
			}
		}
		else {
			for (int x=24; x<=31; ++x) {
				ASMCodeFile << ASMPlayfield[x][y];
			}
		}
	}

	ASMCodeFile << "\nPF2TitleLeft"; // Playfield 2 left
	for (int y=0; y<Height; ++y)
	{
		ASMCodeFile << "\n\t.byte #%";
		for (int x=19; x>=12; --x) {
			ASMCodeFile << ASMPlayfield[x][y];
		}
	}
	ASMCodeFile << "\nPF2TitleRight"; // Playfield 2 right
	for (int y=0; y<Height; ++y)
	{
		ASMCodeFile << "\n\t.byte #%";
		if (Reflected) {
			for (int x=20; x<=27; ++x) {
				ASMCodeFile << ASMPlayfield[x][y];
			}
		}
		else {
			for (int x=39; x>=32; --x) {
				ASMCodeFile << ASMPlayfield[x][y];
			}
		}
	}
}
// Start of program
int main (int argc, char **argv)
{

	cout << "***********************\n";
	cout << "** BMP to Atari 2600 **\n";
	cout << "**   Compiler  V1.2  **\n";
	cout << "**-------------------**\n";
	cout << "**    Programmer:    **\n";
	cout << "**      GeekyLink    **\n";
	cout << "***********************\n";
	cout << "Converts a 24bit BMP to 6502 compatible ASM for use in Atari 2600 games.\n";
	cout << "http://www.gekinzuku.com\n\n";
	
	/* check we have command line options */
    if (argc == 4)
    {
     strcpy(FileName, argv[1]);
     strcpy(ASMFileName, argv[2]);
     if (argv[3][1] == 'p')
        Mode = PLAYFIELD;
     else if (argv[3][1] == 's')
        Mode = SPRITE;
     else {
          cout << "Error: invalid selection for flag 3. Please use -p or -s.";
          cin.ignore();
          exit(0);
     }
     Reflected = false;
     RunScripts();
    }
    else if (argc == 5)
    {
         strcpy(FileName, argv[1]);
         strcpy(ASMFileName, argv[2]);
         if (argv[3][1] == 'p')
            Mode = PLAYFIELD;
         else if (argv[3][1] == 's')
            Mode = SPRITE;
         else {
            cout << "Error: invalid selection for flag 3. Please use -p or -s.";
            cin.ignore();
            exit(0);
         }
         
         if (argv[4][1] == 'r')
            Reflected = true;
         else if (argv[4][1] == 'n')
            Reflected = false;
         else {
            cout << "Error: invalid selection for flag 4. Please use -n or -r.";
            cin.ignore();
            exit(0);
         }
         RunScripts();
         exit(0);
    }
    else if (argc > 1) // If flags but not the right amount
    {
     cout << "When using flags, the usage is:\nBmpTo2600 image.bmp code.asm -p/s -r/n";
     cout << "\n\nImage must be a 24 bit BMP.\n";
     cout << "-p is a playfield, -s is a sprite\n";
     cout << "-r is a reflected playfield, -n is normal.\n";
     cin.ignore();
     exit(0);
    }

	// Asks user if image is a sprite or playfield
	char SelectMode = '0';
	while (SelectMode == '0')
	{
		cout << "Select mode:\n";
		cout << "1. Sprite\n2. Playfield\nMode: ";
		cin.ignore ( cin.rdbuf()->in_avail() );
		cin >> SelectMode;
		if (SelectMode != '1' && SelectMode != '2')
		{
			SelectMode = '0';
			cout << "Error, invalid selection. Try Again.\n\n";
		}
	}
	// Stores it in the mode var
	if (SelectMode == '1')
		Mode = SPRITE;
	else 
    {
         Mode = PLAYFIELD;

	     SelectMode = '0';
	     while (SelectMode == '0')
	     {
		       cout << "Select reflected:\n";
		       cout << "1. Reflected\n2. Not reflected\nChoice: ";
		       cin.ignore ( cin.rdbuf()->in_avail() );
		       cin >> SelectMode;
		       if (SelectMode != '1' && SelectMode != '2')
		       {
			      SelectMode = '0';
                  cout << "Error, invalid selection. Try Again.\n\n";
		       }
	     }

	     if (SelectMode == '1')
		    Reflected = true;
         else Reflected = false;
    }
	
	cin.ignore();

	// Gets filename
	cout << "\nEnter FileName of BMP: ";
	cin.ignore ( cin.rdbuf()->in_avail() );
	cin.getline(FileName, 256);

	cout << "Enter Filename to save ASM code in: ";
	cin.ignore ( cin.rdbuf()->in_avail() );
	cin.getline(ASMFileName, 256);
	
	RunScripts();
	Pause();

	return 0;
}

// Loads the file, and sets up the conversion
void RunScripts()
{
    // Loads the file
	cout << "\nLoading image: " << FileName << endl;
	LoadFileIntoMem(FileName);

	// Puts info into bmp class
	BMP Image;
	if (!Image.LoadFromMem())
	{
		Pause();
		exit(1);
	}

	// Mode to render 
	if (Mode == SPRITE) Image.RenderSprite();
	else Image.RenderPlayfield();

	cout << "\n\n" << ASMFileName << " created successfully!\n";
	delete[] FileMem; 
}

// Loads file into the FileMemory
void LoadFileIntoMem(char * FileName)
{
	ifstream::pos_type FileSize; // File size
	// Reading in the file
	ifstream Image;
	Image.open(FileName, ios::in|ios::binary|ios::ate); // Opens file

	// Did image open correctly?
	if (Image.is_open())
	{
		// Loads the file into memory for easy reading
		FileSize = Image.tellg();
		FileMem = new char [FileSize];
		Image.seekg(0, ios::beg);
		Image.read(FileMem, FileSize);
		Image.close();
	}
	else // Tell user of the error, and exit
	{
		cout << "Error: The file was either misspelled or does not exist.\n";
		Pause();
		delete[] FileMem;
		exit(1);
	}
}

// Pauses the screen, just cause it irritates me typing it :P
void Pause()
{
	cout << "Press Enter to continue...\n";
	cin.ignore ( cin.rdbuf()->in_avail() );
	cin.get();
}
