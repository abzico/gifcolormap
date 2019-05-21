/**
 * Introduction
 * #############
 * Program designed to work with colormap of gif image.
 * If you need to swap target color to the first position of colormap, see tcolorswap at https://github.com/abzico/tcolorswap.
 *
 * This program is designed to work with only 1 single image at a time.
 * You can batch process several images by executing this program via bash script.
 *
 * Warning
 * ########
 * no need for color translation
 * users should spare enough safe slots for new colors to be added
 * so the existing colors in colormap won't be modified for their positions
 * this requires planning before using this program
 *
 * Cli Usage
 * #########
 *  gifcolormap -add-colormap r,g,b|... input-file output-file
 *  ex. gifcolormap -add-color 248,248,12 -add-color 124,224,124 input.gif output.gif
 *
 * command-list
 *  * -add-colormap
 *    Add one more color appending into image's colormap. You can specify multiple of it.
 *    If such color already existed in the colormap, then it will skip, and try to add next one,
 *    if any. It will add each color starting from the end of colormap which is 256th color, then
 *    255th and so on. It does this because we mostly didn't need or know the exact last color
 *    in used in colormap.
 * 
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <gif_lib.h>

typedef struct COLOR COLOR;
struct COLOR
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

// input/output filenames
static const char* input_filename = NULL;
static const char* output_filename = NULL;

// variable to hold colors-input
// will be dynamically allocated according to the number of input colors in parameters
static COLOR* addcolors = NULL;
static int number_colors_input = 0;

// colormap of input image file
static ColorMapObject* colormap;

/**
 * Close decoding gif file.
 * After calling this function, `file` should not be further used anymore.
 */
static void close_dgiffile(GifFileType* file);

/**
 * Close encoding gif file.
 * After calling this function, `file` should not be further used anymore.
 */
static void close_egiffile(GifFileType* file);

/**
 * Free map object.
 * After calling this function, `map` should not be further used anymore.
 */
static void free_mapobject(ColorMapObject* map);

/**
 * Clean up as *neccessary* for input files and object in correct order.
 * Each parameter can be NULL, otherwise it will attemp to clean and free such object accordingly.
 *
 * This function will set NULL to input variables thus users don't have to manually set to NULL again after.
 */
static void cleanup_res(GifFileType** filein, GifFileType** fileout, ColorMapObject** filein_mapobject);

/**
 * Print usage text of this program.
 */
static void cli_print_usage();

/**
 * call exit() with specified status code, but print input `err_str_format` to standard error before exiting.
 */
static void exitnow(int status, const char* err_str_format, ...);

void close_dgiffile(GifFileType* file)
{
  int error_code = 0;
  if (DGifCloseFile(file, &error_code) == GIF_ERROR)
  {
    fprintf(stderr, "Error closing decoding file [error code: %d]\n", error_code);
  }
  // otherwise ok
}

void close_egiffile(GifFileType* file)
{
  int error_code = 0;
  if (EGifCloseFile(file, &error_code) == GIF_ERROR)
  {
    fprintf(stderr, "Error closing encoding file [error code: %d]\n", error_code);
  }
  // otherwise ok
}

void free_mapobject(ColorMapObject* map)
{
  GifFreeMapObject(map);
}

void exitnow(int status, const char* err_str_format, ...)
{
  va_list arg_list;
  va_start(arg_list, err_str_format);

  vfprintf(stderr, err_str_format, arg_list);

  va_end(arg_list);

  // exit with specified status code
  exit(status);
}

void cleanup_res(GifFileType** filein, GifFileType** fileout, ColorMapObject** filein_mapobject)
{
  if (filein != NULL)
  {
    close_dgiffile(*filein);
    *filein = NULL;
  }

  if (fileout != NULL)
  {
    close_egiffile(*fileout);
    *fileout = NULL;
  }

  if (filein_mapobject != NULL)
  {
    free_mapobject(*filein_mapobject);
    *filein_mapobject = NULL;
  }
}

void cli_print_usage()
{
  printf("gifcolormap by Wasin Thonkaew (Angry Baozi Entertainment https://abzi.co\n\n");
  printf("Usage: gifcolormap -add-color r,g,b|... input-imagepath output-imagepath\n\n");
  printf("-- Command list --\n\n");
  printf("  -add-color r,g,b\n\n");
  printf("   This will add a color (RGB) according to value of r,g,b appending into colormap.\n");
  printf("   If the color already existed, then it will skip. You can add multiple colors.\n");
  printf("   It will add color starting at the end of colormap which is 256th, then 255th and so on\n");
  printf("   Ex. gifcolormap -add-color 123,123,123 -add-color 255,255,123 input.gif output.gif\n\n");
}

// TODO: will remove these soon
unsigned char trans_red;
unsigned char trans_green;
unsigned char trans_blue;

int main(int argc, char** argv)
{
  // cli's arguments
  // if number of parameters is less than at least one color to add
  if (argc < 5)
  {
    if (argc >= 2 && strncmp(argv[1], "--help", 6) == 0)
    {
      cli_print_usage();
    }
    else
    {
      fprintf(stderr, "Not enough parameters entered!\n\n");
      cli_print_usage();
    }
    return 1;
  }
  else
  {
    // find number of input color in parameters
    int colors_input_indexes[256]; // maximum can added is 256, but in reality much less
    for (int i=1; i<argc-1; ++i)
    {
      if (strncmp(argv[i], "-add-color", strlen("-add-color")) == 0)
      {
        // save index, and increment number of input colors
        colors_input_indexes[number_colors_input++] = i;
      }
    }

#ifndef NDEBUG
    printf("[DEBUG] number of input colors %d\n", number_colors_input);
    if (number_colors_input > 0)
    {
      printf("[DEBUG] at ");
      for (int i=0; i<number_colors_input; ++i)
      {
        printf("%d ", colors_input_indexes[i]);
      }
    }
    printf("\n");
#endif

    // check if number of parameters is enough
    if (argc < number_colors_input*2 + 3)
    {
      exitnow(1, "Not enough parameters entered! Use --help.\n\n");
    }

    // check if there's at least one color to add
    // otherwise we don't have anything to do
    if (number_colors_input == 0)
    {
      exitnow(1, "There should be at least 1 color to add into color map. Use --help.\n\n");
    }

    // dynamically allocated exact number of colors input needed to hold
    addcolors = malloc(sizeof(COLOR) * number_colors_input);

    // parse input colors to set colors' values
    for (int i=0; i<number_colors_input; ++i)
    {
      // default to black
      uint8_t r=0, g=0, b=0;
      
      sscanf(argv[colors_input_indexes[i] + 1], "%hhu,%hhu,%hhu", &r, &g, &b);
#ifndef NDEBUG
      printf("[DEBUG] R=%hhu, G=%hhu, B=%hhu\n", r, g, b);
#endif
      addcolors[i].r = r;
      addcolors[i].g = g;
      addcolors[i].b = b;
    }

    // input filename is second last
    input_filename = argv[argc-2];
    // output filename is the last
    output_filename = argv[argc-1];

#ifndef NDEBUG
    printf("[DEBUG] input filename '%s'\n", input_filename);
    printf("[DEBUG] output filename '%s'\n", output_filename);
#endif
  }

  // open gif file
  // after it opens, it will fill up gif file structure with information like number of colors in colormap
  GifFileType * gif_filein = DGifOpenFileName(input_filename, NULL);
  if (!gif_filein)
  {
    exitnow(1, "Error opening gif file %s\n", input_filename);
  }

  // create colormap mem to hold colors from colormap
  //
  // Note: we possibly has no need to use GifMakeMapObject() function here but just
  // directly access colormap via ->SColorMap->Colors but we will lose automatic checking
  // of POT (power-of-two) of colors in colormap and memory checking.
  colormap = GifMakeMapObject(gif_filein->SColorMap->ColorCount, gif_filein->SColorMap->Colors);
  if (colormap == NULL)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "Error creating colormap object\n");
  }

  // check if input image has color map at all?
  if (gif_filein->SColorMap == NULL)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "No colormap for %s\n", input_filename);
  }

  // check if colormap has at least 1 color
  if (gif_filein->SColorMap->ColorCount <= 0)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "Error number of colors in colormap is 0\n");
  }

  // global color map
  // global color map is colormap (not histogram) as shown in `identify -verbose ...`
  // color map always in RGB format, and has maximum of 256 colors in a single map
  // note: local color map is not always available
  int num_color_colormap = colormap->ColorCount; 

#ifndef NDEBUG
  printf("[DEBUG] Colors: %d\n", num_color_colormap); 
#endif

  // access to the colormap's colors memory
  // **remember**: to access map object not directly here
  // libgif doesn't offer in-place modification of colormap, thus directly access makes no sense.
  // accessing via map object is safer, and we will use it (as a copy) to write to output image.
  GifColorType* filein_colors = colormap->Colors;

  // start at the end of colormap
  int dupcount=0;
  for (int i=num_color_colormap-1, j=0; i>=num_color_colormap-number_colors_input; --i,++j)
  {
    COLOR acolor = addcolors[j];

    // check if such color already existed in the colormap
    // if so then skip this color
    bool existed = false;
    for (int k=0; k<num_color_colormap; ++k)
    {
      if (acolor.r == filein_colors[k].Red &&
          acolor.g == filein_colors[k].Green &&
          acolor.b == filein_colors[k].Blue)
      {
        existed = true;
        printf("found existing color %hhu,%hhu,%hhu\n", acolor.r, acolor.g, acolor.b);
        break;
      }
    }

    // increment duplicated count, if existed
    // so we don't move index backward in colormap unneccessarily
    if (existed)
    {
      ++dupcount;
    }
    // if not existed, then add such color
    else
    {
      int target_i = i + dupcount;

      filein_colors[target_i].Red = acolor.r;
      filein_colors[target_i].Green= acolor.g;
      filein_colors[target_i].Blue = acolor.b;
    }
  }

  // now we're ready to write to output file
  int error_code;
  GifFileType* gif_fileout = EGifOpenFileName(output_filename, false, &error_code);
  if (gif_fileout == NULL)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "Error opening output file %s to write [error code: %d]\n", output_filename, error_code);
  }

  // set gif version to GIF89
  EGifSetGifVersion(gif_fileout, true);

  // put screen description
  if(EGifPutScreenDesc(gif_fileout,
        gif_filein->SWidth,
        gif_filein->SHeight,
        gif_filein->SColorResolution,
        gif_filein->SBackGroundColor,
        colormap) == GIF_ERROR)
  {
    cleanup_res(&gif_filein, &gif_fileout, &colormap);
    exitnow(1, "Cannot put screen description to output file %s\n", output_filename);
  }

  // read and handle each type of record accordingly
  // until we reach the end of record
  GifRecordType record_type = UNDEFINED_RECORD_TYPE;
  do {
    // read next record
    if (DGifGetRecordType(gif_filein, &record_type) == GIF_ERROR)
    {
      cleanup_res(&gif_filein, &gif_fileout, &colormap);
      exitnow(1, "Error reading next record\n");
    }

    // variables used inside the switch block
    int extcode = 0;
    GifByteType* extension;

    // handle accordingly for each record type
    switch (record_type)
    {
      case IMAGE_DESC_RECORD_TYPE:
        // read in image description from input file
        if (DGifGetImageDesc(gif_filein) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error getting image description\n");
        }

        // put same image description into output file
        // putting modified colormap into output image
        if (EGifPutImageDesc(gif_fileout,
            gif_filein->Image.Left,
            gif_filein->Image.Top,
            gif_filein->Image.Width,
            gif_filein->Image.Height,
            gif_filein->Image.Interlace,
            gif_filein->Image.ColorMap) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error putting image destination\n");
        }

        int i;

        GifPixelType* line = (GifPixelType*)malloc(gif_filein->Image.Width * sizeof(GifPixelType));
        for (i=0; i<gif_filein->Image.Height; ++i)
        {
          if (DGifGetLine(gif_filein, line, gif_filein->Image.Width) == GIF_ERROR)
          {
            cleanup_res(&gif_filein, &gif_fileout, &colormap);
            exitnow(1, "Error getting line from input image\n");
          }

          // no need for color translation
          // users should spare enough safe slots for new colors to be added
          // so the existing colors in colormap won't be modified for their positions
          // this requires planning before using this program

          // put line
          if (EGifPutLine(gif_fileout, line, gif_filein->Image.Width) == GIF_ERROR)
          {
            cleanup_res(&gif_filein, &gif_fileout, &colormap);
            exitnow(1, "Error putting line into output file\n");
          }
        }
        free(line);
        break;

      case EXTENSION_RECORD_TYPE:

        // read extension block
        if (DGifGetExtension(gif_filein, &extcode, &extension) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error reading extension from input file\n");
        }
        // check if extension is not available, then break now
        if (extension == NULL)
          break;

        // start writing the beginning of extension block
        if (EGifPutExtensionLeader(gif_fileout, extcode) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error putting extension leader\n");
        }
        if (EGifPutExtensionBlock(gif_fileout, extension[0], extension + 1) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error putting extension block\n");
        }

        while (extension != NULL)
        {
          if (DGifGetExtensionNext(gif_filein, &extension) == GIF_ERROR)
          {
            cleanup_res(&gif_filein, &gif_fileout, &colormap);
            exitnow(1, "Error getting next extension\n");
          }
          if (extension != NULL)
          {
            if (EGifPutExtensionBlock(gif_fileout, extension[0], extension + 1) == GIF_ERROR)
            {
              cleanup_res(&gif_filein, &gif_fileout, &colormap);
              exitnow(1, "Error putting extension block\n");
            }
          }
        }

        if (EGifPutExtensionTrailer(gif_fileout) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error putting extension block\n");
        }

        break;

      case TERMINATE_RECORD_TYPE:
        break;
      default:
        break;
    }
  } while (record_type != TERMINATE_RECORD_TYPE);

  cleanup_res(&gif_filein, &gif_fileout, &colormap);
  return 0;
}
