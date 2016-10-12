#include "code_gen.h"
#include "cpp.h"
#include "error.h"
#include "scanner.h"
#include "parser.h"

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <string>

#include <fcntl.h>
#if defined(__linux__) || defined(__GUNC__)
#include <unistd.h>
#endif


std::string program;
std::string inFileName;
std::string outFileName;


void Usage()
{
  printf("Usage: wgtcc [options] file...\n"
       "Options: \n"
       "  --help    show this information\n"
       "  -D        define object like macro\n"
       "  -I        add search path\n"
       "  -o        specify output filename\n");
  
  exit(0);
}

// TODO(wgtdkp): gen only assembly code
// or pass args to gcc
int main(int argc, char* argv[])
{
  bool printPreProcessed = false;
  bool printAssembly = false;

  if (argc < 2) {
    Usage();
  }
  program = std::string(argv[0]);
  // Preprocessing
  Preprocessor cpp(&inFileName);
  
  for (auto i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      inFileName = std::string(argv[i]);
      continue;
    }

    switch (argv[i][1]) {
    case 'o':
      if (i + 1 == argc)
        Usage();
      outFileName = argv[++i];
      break;
    case 'I':
      cpp.AddSearchPath(std::string(&argv[i][2]));
      break;
    case 'D': {
      auto def = std::string(&argv[i][2]);
      auto pos = def.find('=');
      std::string macro;
      std::string* replace;
      if (pos == std::string::npos) {
        macro = def;
        replace = new std::string();
      } else {
        macro = def.substr(0, pos);
        replace = new std::string(def.substr(pos + 1));
      }
      cpp.AddMacro(macro, replace); 
    } break;
    case 'P':
      switch (argv[i][2]) {
      case 'P': printPreProcessed = true; break;
      case 'A': printAssembly = true; break;
      default: Error("unrecognized command line option '%s'", argv[i]);
      } break;
    case '-': // --
      switch (argv[i][2]) {
      case 'h': Usage(); break;
      default:
        Error("unrecognized command line option '%s'", argv[i]);
      }
      break;
    case '\0':
    default:
      Error("unrecognized command line option '%s'", argv[i]);
    }
  }

  if (inFileName.size() == 0) {
    Usage();
  }

  //clock_t begin = clock();
  // change current directory
  auto tmpOutFileName = outFileName;
  outFileName = inFileName;
#if defined(_WIN32) || defined(OS_WINDOWS) || defined(_WINDOWS_)
  std::string dir = ".\\";
#else
  std::string dir = "./";
#endif
  auto pos = inFileName.rfind('/');
  if (pos == std::string::npos) {
      pos = inFileName.rfind('\\');
  }
  
  if (pos != std::string::npos) {
    dir = inFileName.substr(0, pos + 1);
    outFileName = inFileName.substr(pos + 1);
  }
  outFileName.back() = 's';
  if (tmpOutFileName.size())
    outFileName = tmpOutFileName;
  cpp.AddSearchPath(dir);

  TokenSequence ts;
  cpp.Process(ts);

  if (printPreProcessed) {
    std::cout << std::endl << "###### Preprocessed ######" << std::endl;
    ts.Print();
  }

  // Parsing
  Parser parser(ts);
  parser.Parse();
  
  // CodeGen
  auto outFile = fopen(outFileName.c_str(), "w");
  assert(outFile);

  Generator::SetInOut(&parser, outFile);
  Generator g;
  g.Gen();

  //clock_t end = clock(); 

  fclose(outFile);

  if (printAssembly) {
    auto str = ReadFile(outFileName);
    std::cout << *str << std::endl;
  }
  
  std::string sys = "gcc -std=c11 -Wall " + outFileName;
  auto ret = system(sys.c_str());

  //std::cout << "time: " << (end - begin) * 1.0f / CLOCKS_PER_SEC << std::endl;
  return ret;
}
