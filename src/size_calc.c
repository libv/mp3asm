#include <stdio.h>
int
main (int argc, char **argv)
{
  int kbps[2][3][16] = {{
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1},
    {0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, -1},
    {0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, -1}
  },{
    {0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, -1},
    {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1},
    {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1}}};
  int frequencies[3][3] = {{44100, 48000, 32000}, 
			   {22050, 24000, 16000}, 
			   {11025, 12000,  8000}};
  
  int version; // 0: mpeg 1; 1: mpeg 2; 2: mpeg 2.5
  int layer;   // 0: layer 1; 1: layer 2; 2: layer 3
  int bitrate; // 0-15: number gotten from header.
  int samples; // layer 1: 384; layer 2/layer 3 mpeg 2: 576; layer 3 mpeg 3: 1152
  int frequency; // 0,1,2
  int size;
  int padding;

  for (version = 0; version < 3; version++) {
    for (layer = 0; layer < 3; layer++) {
      if (!layer)
	samples = 384;
      else if (layer == 1)
	samples = 576;
      else if (!version)
	samples = 1152;
      else
	samples = 576;
      for (frequency = 0; frequency < 3; frequency++) {
	for (padding = 0; padding < 2; padding++) {
	  if (padding && !layer)
	    padding = 4;
	  printf ("version: %d; layer: %d; frequency: %d; padding: %d\n", version, layer, frequency, padding);
	  for (bitrate = 0; bitrate < 15; bitrate++) {
	    size = (kbps[version >> 1][layer][bitrate] * 125 * samples) / frequencies[version][frequency] + padding; 
	    printf("%4d, ", size);
	  }
	  printf("\n");
	}
	printf ("\n");
      }
      printf ("\n");
    }
  }
  return (0);
}
