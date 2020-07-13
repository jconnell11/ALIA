// tais_img.cpp : converts base64 blob in JSON to JPEG image file

#include <stdio.h>
#include <string.h>
#include <conio.h>


//= Get 24 bit value (3 x 8 bits) as 4 x 6 bit characters.
// for input sequence a,b,c,d generates val = a:b:c:d
// returns 1 if more file, 0 if end reached (EOF or quote)

int get24 (unsigned long& val, FILE *in)
{
  char c;
  int sh, v6;

  // add 6 bit chunks from the top down
  val = 0;
  for (sh = 18; sh >= 0; sh -= 6)
  {
    // check for end of encoded blob
    if ((c = fgetc(in)) == EOF)
      return 0;
    if (c == '"')
      return 0;

    // convert character to 6 bit value
    if ((c >= 'A') && (c <= 'Z'))
      v6 = (int)(c - 'A');
    else if ((c >= 'a') && (c <= 'z'))
      v6 = (int)(c - 'a') + 26;
    else if ((c >= '0') && (c <= '9'))
      v6 = (int)(c - '0') + 52;
    else if (c == '+')
      v6 = 62;
    else if (c == '/')
      v6 = 63;
    else
      return 0;

    // merge into 24 bit value
    val |= (v6 << sh);
  }
  return 1;
}


//= Put 3 bytes of 24 bit value to output file.
// for val = A:B:C outputs sequence A,B,C
// returns 1 for success, 0 for problem

int put24 (FILE *out, unsigned long val)
{
  if (fputc((val >> 16) & 0xFF, out) == EOF)
    return 0;
  if (fputc((val >> 8) & 0xFF, out) == EOF)
    return 0;
  if (fputc(val & 0xFF, out) == EOF)
    return 0;
  return 1;
}


//= Convert whole payload of input to a JPEG image.

int main (int argc, char *argv[])
{
  char key[80] = "payload";
  char fname[80] = "Image_Sample.json";
  FILE *in, *out;
  unsigned long val;
  int pos = 0;
  char c;

  // open input and output files (binary)
  if (argc > 1)
    strcpy_s(fname, argv[1]);
  if (fopen_s(&in, fname, "rb") != 0)
    return 0;
  if (fopen_s(&out, "base64.jpg", "wb") != 0)
  {
    fclose(in);
    return 0;
  }

  // look for payload marker 
  while ((c = fgetc(in)) != EOF)
    if (c != key[pos])
      pos = 0;
    else if (++pos >= 7)
      break;

  // skip close quote then look for next quote
  if (fgetc(in) != EOF)
    while ((c = fgetc(in)) != EOF)
      if (c == '"')
        break;

  // convert bulk up until next quote
  while (get24(val, in) > 0)
    put24(out, val);

  // cleanup
  fclose(in);
  fclose(out);
  printf("Image converted\n");
  return 1;
}



