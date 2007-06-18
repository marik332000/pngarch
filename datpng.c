#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include "datpng.h"

static int img_width = 250;
static int img_height = 0;
static int verbose_flag = 0;
char *progname;

int decode_dat(char *);
int encode_dat(char *);
int auto_dat(char *filename);

int print_usage(int exit_status)
{
  printf("Usage: %s [options] [file]\n\n", progname);
  printf("Options:\n\n");
  printf("  -c, --create     Create data PNG\n");
  printf("  -x, --extract    Extract data PNG\n");
  printf("  -w, --width      Width of the image (default %d)\n",
	 img_width);
  printf("  -h, --height     Height of the image (default %d)\n", 
	 img_height);
  printf("  -t, --list       List internal filename\n");
  printf("  -V, --verbose    Enable verbose output\n");
  printf("  -v, --version    Display version information\n");
  printf("  -h, --help       Display this help text\n");

  exit(exit_status);
}

int main(int argc, char **argv)
{
  progname = argv[0];
  
  int auto_mode = 1; /* Auto detect mode. */
  int create_mode = 0; /* Create archive. */
  int extract_mode = 0; /* Extract archive. */

  int c; /* Current option. */
  
  while (1)
    {
      static struct option long_options[] =
	{
	  /* These options set a flag. */
	  {"help",    no_argument,       0, 'H'},
	  {"version", no_argument,       0, 'v'},
	  {"verbose", no_argument,       0, 'V'},
	  {"extract", no_argument,       0, 'x'},
	  {"create",  no_argument,       0, 'c'},
	  {"width",   required_argument, 0, 'w'},
	  {"height",  required_argument, 0, 'h'},
	  {"list",    no_argument,       0, 't'},
	  {0, 0, 0, 0}
	};
      
      /* getopt_long stores the option index here. */
      int option_index = 0;
      
      c = getopt_long (argc, argv, "hvVxcw:h:t",
		       long_options, &option_index);
      
      /* Detect the end of the options. */
      if (c == -1)
	break;
      
      switch (c)
	{
             case 'H':
	       print_usage(EXIT_SUCCESS);
               break;

             case 'v':
               break;
     
             case 'V':
	       verbose_flag = 1;
               break;
     
             case 'x':
	       if (!create_mode)
		 {
		   extract_mode = 1;
		   auto_mode = 0;
		 }
	       else
		 {	
		   fprintf(stderr, 
			  "%s: cannot specify more than one -xc option\n", 
			  progname);
		   print_usage(EXIT_FAILURE);
		 }
               break;
     
             case 'c':
	       if (!extract_mode)
		 {
		   create_mode = 1;
		   auto_mode = 0;
		 }
	       else
		 {	
		   fprintf(stderr, 
			   "%s: cannot specify more than one -xc option\n", 
			  progname);
		   print_usage(EXIT_FAILURE);
		 }
               break;
     
             case 'w':
	       img_width = atoi(optarg);
               break;
     
             case 'h':
	       img_height = atoi(optarg);
               break;
     
             case 't':
               break;
     
             case '?':
               print_usage(EXIT_FAILURE);
               break;
     
             default:
               abort ();
             }
         }

  /* Extract from the archive. */
  if (extract_mode == 1)
    {
      int i;
      for (i = optind; i < argc; i++)
	decode_dat(argv[i]);

      if (argc == optind)
	decode_dat("-");
    }
  
  /* Create new archive. */
  if (create_mode == 1)
    {
      int i;
      for (i = optind; i < argc; i++)
	encode_dat(argv[i]);

      if (argc == optind)
	encode_dat("-");
    }

  /* Automode - method based on filename extension. */
  if (auto_mode)
    {
      if (argc == optind)
	encode_dat("-");
      else
	{
	  int i;
	  for (i = optind; i < argc; i++)
	    {
	      auto_dat(argv[i]);      
	    }
	}
    }
  
  exit(EXIT_SUCCESS);
}

int decode_dat(char *filename)
{
  /* Set the necessary meta data. */
  datpng_info data_info;
  data_info.bit_depth = 8;
  data_info.checksum = 0;
  data_info.x_pos = 0;
  data_info.y_pos = 0;
  data_info.data_width = img_width;
  data_info.data_height = 0;
  data_info.png_width = img_width;
  data_info.png_height = 0;

  FILE *fp;
  if (strcmp(filename, "-") == 0)
    fp = stdin;
  else
    {
      fp = fopen(filename, "rb");
      if (fp == NULL)	
	{
	  fprintf(stderr, "%s: Failed to open file %s - %s", 
		  progname, filename, strerror(errno));
	  exit(EXIT_FAILURE);
	}
    }
  
  /* Get data from the PNG file. */
  size_t buffer_size = 0;
  void *buffer;
  datpng_read(fp, &data_info, &buffer, &buffer_size);
  //  printf(buffer + strlen(buffer) + 1);

  if (strcmp(filename, "-") != 0)
    fclose(fp);
  
  /* Unload the data into the file named by the internal filename. */
  char *outfile = strdup(buffer);
  if (outfile == NULL)
    {
      fprintf(stderr, "%s: failed strdup - %s", progname, strerror(errno));
      exit(EXIT_FAILURE);
    }
  
  FILE *fw;
  if (strlen(outfile) == 0)
    fw = stdout;
  else
    {
      fw = fopen(outfile, "wb");
      if (fp == NULL)	
	{
	  fprintf(stderr, "%s: Failed to open file %s - %s", 
		  progname, outfile, strerror(errno));
	  exit(EXIT_FAILURE);
	}
    }
  
  int boff = strlen(outfile) + 1;
  fwrite(buffer + boff, buffer_size - boff, 1, fw);
  
  if (strlen(outfile) != 0)
    fclose(fw);
  
  if (verbose_flag && strlen(outfile) != 0)
    fprintf(stderr, "Extracted file %s\n", outfile);
  
  return 0;
}

int encode_dat(char *infile)
{
  if (verbose_flag)
    fprintf(stderr, "Encoding %s\n", infile);
  
  char *outfile; /* Output filename. */
  
  /* Prepare input buffer */
  size_t read_size = 1024; /* Reading in 1 kbyte at a time. */
  size_t buffer_max = 1024 * 64; /* Initial buffer at 64 kbytes. */
  size_t buffer_size = 0;
  void *buffer = malloc(buffer_max);
  
  FILE *fin; /* input file pointer */
  if (strcmp(infile, "-") == 0)
    {
      outfile = "-";
      buffer_size += 1;
      strncpy(buffer, "", buffer_max);
      fin = stdin;
    }
  else
    {
      buffer_size += (strlen(infile) + 1);
      strncpy(buffer, infile, buffer_max);

      fin = fopen(infile, "rb");
      if (fin == NULL)
	{
	  fprintf(stderr, "%s: Failed to open file %s - %s", 
		  progname, infile, strerror(errno));
	  exit(EXIT_FAILURE);
	}
      
      outfile = (char *)malloc(strlen(infile) + 5);
      if (outfile == NULL)
	{
	  fprintf(stderr, "%s: Failed to malloc outfilename - %s", 
		  progname, strerror(errno));
	  exit(EXIT_FAILURE);
	}
      snprintf(outfile, strlen(infile) + 5, "%s.png", infile);
    }

  /* Read file into the buffer. */
  if (verbose_flag)
    fprintf(stderr, "Loading file %s\n", infile);
  while (!feof(fin))
    {      
      /* Double buffer size when too small. */
      if (buffer_size >= buffer_max - read_size)
	{
	  buffer_max *= 2;
	  buffer = realloc(buffer, buffer_max);
	}

      size_t amt;
      amt = fread(buffer + buffer_size, 1, read_size, fin);
      buffer_size += amt;
    }
  
  if (strcmp(infile, "-") != 0)
    fclose(fin);
  
  /* Setup meta data. */
  datpng_info data_info; 
  data_info.bit_depth = 8;
  data_info.checksum = 0;
  data_info.x_pos = 0;
  data_info.y_pos = 0;
  data_info.data_width = img_width;
  data_info.data_height = 0;
  data_info.png_width = img_width;
  data_info.png_height = 0;
  
  FILE *fp;
  if (strcmp(outfile, "-") == 0)
    fp = stdout;
  else
    {
      fp = fopen(outfile, "wb");
      if (fp == NULL)
    	{
	  fprintf(stderr, "%s: Failed to open file %s - %s", 
		  progname, infile, strerror(errno));
	  exit(EXIT_FAILURE);
	}
    }

  /* Finally we get to write the data out to a PNG. */
  datpng_write(fp, &data_info, buffer, buffer_size);

  if (strcmp(outfile, "-") != 0)
    fclose(fp);
  
  free(buffer);
  
  return 0;
}

int auto_dat(char *filename)
{
  if (strstr(filename, ".png") == filename + strlen(filename) - 4 ||
      strstr(filename, ".PNG") == filename + strlen(filename) - 4)
    return decode_dat(filename);
  else
    return encode_dat(filename);
}
