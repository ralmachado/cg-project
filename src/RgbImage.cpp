/*
 *
 * RgbImage.h - release 4.0.  May 1, 2018.
 *
 * Author: Samuel R. Buss
 *
 * Software accompanying the book
 *		3D Computer Graphics: A Mathematical Introduction with OpenGL,
 *		by S. Buss, Cambridge University Press, 2003.
 *
 * Software is "as-is" and carries no warranty.  It may be used without
 *   restriction, but if you modify it, please change the filenames to
 *   prevent confusion between different versions.  Please acknowledge
 *   all use of the software in any publications or products based on it.
 *
 * Bug reports: Sam Buss, sbuss@ucsd.edu.
 * Web page: http://math.ucsd.edu/~sbuss/MathCG
 *
 */

// This tells the Visual C++ 2005 compiler to allow use of fopen, sscnaf, strcpy, etc.
// Undocumented: This must be included *before* the #include of stdio.h (!!)
#define _CRT_SECURE_NO_DEPRECATE 1

#include "include/RgbImage.h"

#ifndef RGBIMAGE_DONT_USE_OPENGL
#if defined(_WIN32)			// If on windows, need this for gl.h
#include <windows.h>
#endif  // defined(_WIN32)
#include "GL/gl.h"
#endif
#ifndef BI_RGB
#define BI_RGB 0
#endif

RgbImage::RgbImage( int numRows, int numCols )
{
    AllocateImageData(numRows, numCols);

    // Zero out the image
	unsigned char* c = ImagePtr;
	int rowLen = GetNumBytesPerRow();
	for ( int i=0; i<NumRows; i++ ) {
		for ( int j=0; j<rowLen; j++ ) {
			*(c++) = 0;
		}
	}
	ErrorCode = NoError;
}
/* *************************************************************************
 * Copy constructor - also makes a copy of the bit image.
 * Modified from code provided by William Joel (Western Connecticut State Univ.)
   *************************************************************************/
RgbImage::RgbImage(const RgbImage *image) {
	NumCols = image->GetNumCols();
	NumRows = image->GetNumRows();
	long size = NumRows*GetNumBytesPerRow();
	unsigned char *fromImage = image->ImagePtr;
	ImagePtr = new unsigned char[size];
	if ( !ImagePtr ) {
		fprintf(stderr, "Unable to allocate memory for %ld x %ld bitmap.\n", 
				NumCols, NumRows);
		Reset();
		ErrorCode = MemoryError;
	}
	for (long i=0; i < size; i++){
		ImagePtr[i] = fromImage[i];
	}
	ErrorCode = NoError;
}


/* ********************************************************************
 *  LoadBmpFile
 *  Read into memory an RGB image from an uncompressed BMP file.
 *  Return true for success, false for failure.  Error code is available
 *     with a separate call.
 *  Author: Sam Buss December 2001.
 **********************************************************************/

bool RgbImage::LoadBmpFile( const char* filename ) 
{  
	Reset();
	FILE* infile = fopen( filename, "rb" );		// Open for reading binary data
	if ( !infile ) {
		fprintf(stderr, "Unable to open file: %s\n", filename);
		ErrorCode = OpenError;
		return false;
	}

	bool fileFormatOK = false;
	int bChar = fgetc( infile );
	int mChar = fgetc( infile );
	if ( bChar=='B' && mChar=='M' ) {			// If starts with "BM" for "BitMap"
		skipChars(infile, 4 + 2 + 2);			// Skip 3 fields (size of file and two reserved fields)
		int offset = readLong(infile);			// Offset to the bitmap table
		int headerSize = readLong(infile);		// Size of the Bitmap header
		NumCols = readLong( infile );
		NumRows = readLong( infile );
		skipChars( infile, 2 );					// Skip one field (number of color planes)
		int bitsPerPixel = readShort( infile );
		int bytesRead = 30;						// 2 + 4 + 2 + 2 + 4 + 4 + 4 + 4 + 2 + 2
		int compressionMethod = BI_RGB;
		if (headerSize >= 40) {
			compressionMethod = readLong(infile);
			bytesRead += 4;
		}
		skipChars(infile, offset - bytesRead);

		if ( NumCols>0 && NumCols<=100000 && NumRows>0 && NumRows<=100000  
			&& bitsPerPixel==24 && compressionMethod ==BI_RGB && !feof(infile) ) {
			fileFormatOK = true;
		}
	}
	if ( !fileFormatOK ) {
		Reset();
		ErrorCode = FileFormatError;
		fprintf(stderr, "Not a valid 24-bit, BI_RGB, bitmap file: %s.\n", filename);
		fclose ( infile );
		return false;
	}

	// Allocate memory
    if (!AllocateImageData(NumRows, NumCols)) {
		fclose ( infile );
		return false;
	}

	unsigned char* cPtr = ImagePtr;
	for ( int i=0; i<NumRows; i++ ) {
		int j;
		for ( j=0; j<NumCols; j++ ) {
			*(cPtr+2) = (unsigned char)fgetc( infile );	// Blue color value
			*(cPtr+1) = (unsigned char)fgetc( infile );	// Green color value
			*cPtr = (unsigned char)fgetc( infile );		// Red color value
			cPtr += 3;
		}
		int k=3*NumCols;					// Num bytes already read
		for ( ; k<GetNumBytesPerRow(); k++ ) {
			fgetc( infile );				// Read and ignore padding;
			*(cPtr++) = 0;
		}
	}
	if ( feof( infile ) ) {
		fprintf( stderr, "Premature end of file: %s.\n", filename );
		Reset();
		ErrorCode = ReadError;
		fclose ( infile );
		return false;
	}
	fclose( infile );	// Close the file
	ErrorCode = NoError;
	return true;
}

short RgbImage::readShort( FILE* infile )
{
	// read a 16 bit integer
    unsigned char lowByte = (unsigned char)fgetc(infile);			// Read the low order byte (little endian form)
    unsigned char hiByte = (unsigned char)fgetc(infile);			// Read the high order byte

	// Pack together
	short ret = hiByte;
	ret <<= 8;
	ret |= lowByte;
	return ret;
}

long RgbImage::readLong( FILE* infile )
{  
	// Read in 32 bit integer
    unsigned char byte0 = (unsigned char)fgetc(infile);			// Read bytes, low order to high order
    unsigned char byte1 = (unsigned char)fgetc(infile);
    unsigned char byte2 = (unsigned char)fgetc(infile);
    unsigned char byte3 = (unsigned char)fgetc(infile);

	// Pack together
	long ret = byte3;
	ret <<= 8;
	ret |= byte2;
	ret <<= 8;
	ret |= byte1;
	ret <<= 8;
	ret |= byte0;
	return ret;
}

void RgbImage::skipChars( FILE* infile, int numChars )
{
	for ( int i=0; i<numChars; i++ ) {
		fgetc( infile );
	}
}

/* ********************************************************************
 *  WriteBmpFile
 *  Write an RGB image to an uncompressed BMP file.
 *  Return true for success, false for failure.  Error code is available
 *     with a separate call.
 *  Author: Sam Buss, January 2003.
 **********************************************************************/

bool RgbImage::WriteBmpFile( const char* filename )
{
	FILE* outfile = fopen( filename, "wb" );		// Open for reading binary data
	if ( !outfile ) {
		fprintf(stderr, "Unable to open file: %s\n", filename);
		ErrorCode = OpenError;
		return false;
	}

	fputc('B',outfile);
	fputc('M',outfile);
	int rowLen = GetNumBytesPerRow();
	writeLong( 40+14+NumRows*rowLen, outfile );	// Length of file
	writeShort( 0, outfile );					// Reserved for future use
	writeShort( 0, outfile );
	writeLong( 40+14, outfile );				// Offset to pixel data
	writeLong( 40, outfile );					// header length
	writeLong( NumCols, outfile );				// width in pixels
	writeLong( NumRows, outfile );				// height in pixels (pos for bottom up)
	writeShort( 1, outfile );		// number of planes
	writeShort( 24, outfile );		// bits per pixel
	writeLong( 0, outfile );		// no compression
	writeLong( 0, outfile );		// not used if no compression
	writeLong( 0, outfile );		// Pixels per meter
	writeLong( 0, outfile );		// Pixels per meter
	writeLong( 0, outfile );		// unused for 24 bits/pixel
	writeLong( 0, outfile );		// unused for 24 bits/pixel

	// Now write out the pixel data:
	unsigned char* cPtr = ImagePtr;
	for ( int i=0; i<NumRows; i++ ) {
		// Write out i-th row's data
		int j;
		for ( j=0; j<NumCols; j++ ) {
			fputc( *(cPtr+2), outfile);		// Blue color value
			fputc( *(cPtr+1), outfile);		// Blue color value
			fputc( *(cPtr+0), outfile);		// Blue color value
			cPtr+=3;
		}
		// Pad row to word boundary
		int k=3*NumCols;					// Num bytes already read
		for ( ; k<GetNumBytesPerRow(); k++ ) {
			fputc( 0, outfile );				// Read and ignore padding;
			cPtr++;
		}
	}

	fclose( outfile );	// Close the file
	ErrorCode = NoError;
	return true;
}

void RgbImage::writeLong( long data, FILE* outfile )
{  
	// Read in 32 bit integer
	unsigned char byte0, byte1, byte2, byte3;
	byte0 = (unsigned char)(data&0x000000ff);		// Write bytes, low order to high order
	byte1 = (unsigned char)((data>>8)&0x000000ff);
	byte2 = (unsigned char)((data>>16)&0x000000ff);
	byte3 = (unsigned char)((data>>24)&0x000000ff);

	fputc( byte0, outfile );
	fputc( byte1, outfile );
	fputc( byte2, outfile );
	fputc( byte3, outfile );
}

void RgbImage::writeShort( short data, FILE* outfile )
{  
	// Read in 32 bit integer
	unsigned char byte0, byte1;
	byte0 = data&0x000000ff;		// Write bytes, low order to high order
	byte1 = (data>>8)&0x000000ff;

	fputc( byte0, outfile );
	fputc( byte1, outfile );
}


/*********************************************************************
 * SetRgbPixel routines allow changing the contents of the RgbImage. *
 *********************************************************************/

void RgbImage::SetRgbPixelf( long row, long col, double red, double green, double blue )
{
	SetRgbPixelc( row, col, doubleToUnsignedChar(red), 
							doubleToUnsignedChar(green),
							doubleToUnsignedChar(blue) );
}

void RgbImage::SetRgbPixelc( long row, long col,
				   unsigned char red, unsigned char green, unsigned char blue )
{
	assert ( row<NumRows && col<NumCols );
	unsigned char* thePixel = GetRgbPixel( row, col );
	*(thePixel++) = red;
	*(thePixel++) = green;
	*(thePixel) = blue;
}


unsigned char RgbImage::doubleToUnsignedChar( double x )
{
	if ( x>=1.0 ) {
		return (unsigned char)255;
	}
	else if ( x<=0.0 ) {
		return (unsigned char)0;
	}
	else {
		return (unsigned char)(x*255.0);		// Rounds down
	}
}

bool RgbImage::AllocateImageData(int numRows, int numCols)
{
    NumRows = numRows;
    NumCols = numCols;
    ImagePtr = new unsigned char[NumRows*GetNumBytesPerRow()];
    if (!ImagePtr) {
        fprintf(stderr, "Unable to allocate memory for %ld x %ld buffer.\n",
            NumRows, NumCols);
        Reset();
        ErrorCode = MemoryError;
        return false;
    }
    return true;
}


// Bitmap file format  (24 bit/pixel form)		BITMAPFILEHEADER
// Header (14 bytes)
//	 2 bytes: "BM"
//   4 bytes: long int, file size
//   4 bytes: reserved (actually 2 bytes twice)
//   4 bytes: long int, offset to raster data
// Info header (40 bytes)						BITMAPINFOHEADER
//   4 bytes: long int, size of info header (=40)
//	 4 bytes: long int, bitmap width in pixels
//   4 bytes: long int, bitmap height in pixels
//   2 bytes: short int, number of planes (=1)
//   2 bytes: short int, bits per pixel
//   4 bytes: long int, type of compression (not applicable to 24 bits/pixel)
//   4 bytes: long int, image size (not used unless compression is used)
//   4 bytes: long int, x pixels per meter
//   4 bytes: long int, y pixels per meter
//   4 bytes: colors used (not applicable to 24 bit color)
//   4 bytes: colors important (not applicable to 24 bit color)
// "long int" really means "unsigned long int"
// Pixel data: 3 bytes per pixel: RGB values (in reverse order).
//	Rows padded to multiples of four.


#ifndef RGBIMAGE_DONT_USE_OPENGL

bool RgbImage::LoadFromOpenglBuffer()					// Load the bitmap from the current OpenGL buffer
{
	GLint viewportData[4];
	glGetIntegerv( GL_VIEWPORT, viewportData );
	int vWidth = viewportData[2];
	int vHeight = viewportData[3];
	
	if ( ImagePtr==0 ) { // If no memory allocated
        bool noAllocError = AllocateImageData(vHeight, vWidth);
        if (!noAllocError) {
            return false;
        }
	}
	assert ( vWidth>=NumCols && vHeight>=NumRows );
	GLint oldGlRowLen;
	if ( vWidth > NumCols ) {
		glGetIntegerv( GL_PACK_ROW_LENGTH, &oldGlRowLen );
		glPixelStorei( GL_PACK_ROW_LENGTH, NumCols );
	}
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	// Get the frame buffer data.
	glReadPixels( 0, 0, NumCols, NumRows, GL_RGB, GL_UNSIGNED_BYTE, ImagePtr);

	// Restore the row length in glPixelStorei  (really ought to restore alignment too).
	if ( vWidth > NumCols ) {
		glPixelStorei( GL_PACK_ROW_LENGTH, oldGlRowLen );
	}	
	ErrorCode = NoError;
	return true;
}

// Draw the bitmap into the current OpenGL buffer
//    Always starts at the position (0,0).
bool RgbImage::DrawToOpenglBuffer()	
{
	GLint viewportData[4];
	glGetIntegerv( GL_VIEWPORT, viewportData );
	int vWidth = viewportData[2];
	int vHeight = viewportData[3];
	
	if ( !ImageLoaded() ) { // If no memory allocated
		assert ( false && "RgbImage.cpp error: No RGB Image for DrawToOpenglBuffer()." );
		ErrorCode = WriteError;
		return false;
	}

	assert ( vWidth>=NumCols && vHeight>=NumRows );
	GLint oldGlRowLen;			
	if ( vWidth > NumCols ) {
		glGetIntegerv( GL_UNPACK_ROW_LENGTH, &oldGlRowLen );
		glPixelStorei( GL_UNPACK_ROW_LENGTH, NumCols );
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// Upload the frame buffer data.
	glRasterPos2i(0,0);		// Position at base of window
	glDrawPixels( NumCols, NumRows, GL_RGB, GL_UNSIGNED_BYTE, ImagePtr);

	// Restore the row length in glPixelStorei  (really ought to restore alignment too).
	if ( vWidth > NumCols ) {
		glPixelStorei( GL_UNPACK_ROW_LENGTH, oldGlRowLen );
	}	
	ErrorCode = NoError;
	return true;
}

#endif   // RGBIMAGE_DONT_USE_OPENGL