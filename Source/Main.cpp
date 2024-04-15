/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libmseed/libmseed.h"

#define SAMPLE_RATE 96000.0f
#define BIT_RATE 24
#define FILE_LENGTH (5.0f * SAMPLE_RATE)
 
#define VERSION "[libmseed " LIBMSEED_VERSION " example]"
#define PACKAGE "mseedview"
 
static int8_t verbose = 1;
static int8_t ppackets = 0;
static int8_t basicsum = 0;
static int printdata = 0;
static char *inputfile = NULL;
 
static int parameter_proc (int argcount, char **argvec);
static void usage (void);

//==============================================================================
int main (int argc, char* argv[])
{

//    if (argc != 3) {
//        std::cerr << "Usage: " << argv[0] << " <mseed-file> <wav-file>" << std::endl;
//        return 1;
//    }
//
//    juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
    
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mseed-file>" << std::endl;
        return 1;
    }
    juce::File inputFile = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    
    juce::File outputFile = inputFile.withFileExtension(".wav");
    if(outputFile.existsAsFile()) {
        outputFile.deleteFile();
    }
    
    juce::AudioBuffer<float> buffer = juce::AudioBuffer<float>(1, FILE_LENGTH);
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset (format.createWriterFor (new juce::FileOutputStream (outputFile),
                                          SAMPLE_RATE,
                                          buffer.getNumChannels(),
                                          BIT_RATE,
                                          {},
                                          0));
    
    
    
    MS3Record *msr = NULL;
  uint32_t flags = 0;
 
  int64_t totalrecs = 0;
  int64_t totalsamps = 0;
  int retcode;
 
  /* Process command line arguments */
//  if (parameter_proc (argc, argv) < 0)
//    return -1;
// 
  printdata = 2;
    inputfile = argv[1];
    
  /* Set flag to validate CRCs when reading */
  flags |= MSF_VALIDATECRC;
 
  /* Parse byte range from file/URL path name if present */
  flags |= MSF_PNAMERANGE;
 
  /* Set flag to unpack data if printing samples */
  if (printdata)
    flags |= MSF_UNPACKDATA;
 
  /* Enable accumulation of up to 10 error and warning messages */
  ms_rloginit (NULL, NULL, NULL, NULL, 10);
 
  /* Loop over the input file record by record */
  while ((retcode = ms3_readmsr (&msr, inputfile, flags, verbose)) == MS_NOERROR)
  {
    totalrecs++;
    totalsamps += msr->samplecnt;
 
    msr3_print (msr, ppackets);
 
    /* Print data samples */
    if (printdata && msr->numsamples > 0)
    {
        buffer.setSize(1, (int) msr->numsamples);
        buffer.clear();
      int line, col, cnt, samplesize;
      long long lines = (msr->numsamples / 6) + 1;
      void *sptr;
 
      if ((samplesize = ms_samplesize (msr->sampletype)) == 0)
      {
        ms_log (2, "Unrecognized sample type: '%c'\n", msr->sampletype);
      }
 
      for (cnt = 0, line = 0; line < lines; line++)
      {
        for (col = 0; col < 6; col++)
        {
          if (cnt < msr->numsamples)
          {
            sptr = (char *)msr->datasamples + (cnt * samplesize);
 
#define INT_FACTOR 0.00000015625f
#define INT_OFFSET 0
              
              double val = 0;
            if (msr->sampletype == 'i') {
              ms_log (0, "%10d  ", *(int32_t *)sptr);
              val = ((float) (*(int32_t *)sptr) * INT_FACTOR) + INT_OFFSET;
            } else if (msr->sampletype == 'f') {
              ms_log (0, "%10.8g  ", *(float *)sptr);
              val = *(float *)sptr;
            } else if (msr->sampletype == 'd') {
              ms_log (0, "%10.10g  ", *(double *)sptr);
              val =  *(double *)sptr;
            }
              buffer.setSample(0, cnt, val);
            cnt++;
          }
        }
        ms_log (0, "\n");
 
        
      }
        
        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
    }
  }
 
  /* Emit all accumulated warning and error messages */
  ms_rlog_emit (NULL, 0, verbose);
 
  /* Make sure everything is cleaned up */
  ms3_readmsr (&msr, NULL, 0, 0);
 
  
    ms_log (0, "Records: %" PRId64 ", Samples: %" PRId64 "\n",
            totalrecs, totalsamps);
    
    
    

    writer->flush();
    
    return 0;
}


/***************************************************************************
 * parameter_proc():
 * Process the command line arguments.
 *
 * Returns 0 on success, and -1 on failure
 ***************************************************************************/
static int
parameter_proc (int argcount, char **argvec)
{
  int optind;
 
  /* Process all command line arguments */
  for (optind = 1; optind < argcount; optind++)
  {
    if (strcmp (argvec[optind], "-V") == 0)
    {
      ms_log (1, "%s version: %s\n", PACKAGE, VERSION);
      exit (0);
    }
    else if (strcmp (argvec[optind], "-h") == 0)
    {
      usage ();
      exit (0);
    }
    else if (strncmp (argvec[optind], "-v", 2) == 0)
    {
      verbose += strspn (&argvec[optind][1], "v");
    }
    else if (strncmp (argvec[optind], "-p", 2) == 0)
    {
      ppackets += strspn (&argvec[optind][1], "p");
    }
    else if (strncmp (argvec[optind], "-d", 2) == 0)
    {
      printdata = 1;
    }
    else if (strncmp (argvec[optind], "-D", 2) == 0)
    {
      printdata = 2;
    }
    else if (strcmp (argvec[optind], "-s") == 0)
    {
      basicsum = 1;
    }
    else if (strncmp (argvec[optind], "-", 1) == 0 &&
             strlen (argvec[optind]) > 1)
    {
      ms_log (2, "Unknown option: %s\n", argvec[optind]);
      exit (1);
    }
    else if (inputfile == NULL)
    {
      inputfile = argvec[optind];
    }
    else
    {
      ms_log (2, "Unknown option: %s\n", argvec[optind]);
      exit (1);
    }
  }
 
  /* Make sure an input file was specified */
  if (!inputfile)
  {
    ms_log (2, "No input file was specified\n\n");
    ms_log (1, "%s version %s\n\n", PACKAGE, VERSION);
    ms_log (1, "Try %s -h for usage\n", PACKAGE);
    exit (1);
  }
 
  /* Add program name and version to User-Agent for URL-based requests */
  if (libmseed_url_support() && ms3_url_useragent(PACKAGE, VERSION))
    return -1;
 
  /* Report the program version */
  if (verbose)
    ms_log (1, "%s version: %s\n", PACKAGE, VERSION);
 
  return 0;
} /* End of parameter_proc() */
 
/***************************************************************************
 * usage():
 * Print the usage message and exit.
 ***************************************************************************/
static void
usage (void)
{
  fprintf (stderr, "%s version: %s\n\n", PACKAGE, VERSION);
  fprintf (stderr, "Usage: %s [options] file\n\n", PACKAGE);
  fprintf (stderr,
           " ## Options ##\n"
           " -V             Report program version\n"
           " -h             Show this usage message\n"
           " -v             Be more verbose, multiple flags can be used\n"
           " -p             Print details of header, multiple flags can be used\n"
           " -d             Print first 6 sample values\n"
           " -D             Print all sample values\n"
           " -s             Print a basic summary after processing a file\n"
           "\n"
           " file           File of miniSEED records\n"
           "\n");
} /* End of usage() */
