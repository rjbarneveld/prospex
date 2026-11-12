/*
   read_config_checked_v2.inc

   Deliberately NOT wrapped in an extra { ... } block.
   read_tables.inc in the existing model reuses inString, inDummy and
   readcounter, so these names must remain in main() scope after inclusion.
*/
if(argc < 2 || argv[1] == nullptr)
{
    std::cerr
        << "Usage: " << argv[0] << " <input.config>" << std::endl;
    return 1;
}

string inString;
string inDummy;
int readcounter = 0;

fin.open(argv[1]);
if(!fin.is_open())
{
    std::cerr
        << "Could not open configuration file: "
        << argv[1] << std::endl;
    return 1;
}

try
{
    while(fin >> inDummy >> inString)
    {cout<<readcounter<<" "<<inDummy<<" "<<inString<<endl;
        if(readcounter == 0)       ioFolder = inString;
        else if(readcounter == 1)  filename_CLIMA = inString;
        else if(readcounter == 2)  nrDays = stoi(inString);
        else if(readcounter == 3)  filename_PARAMS = inString;
        else if(readcounter == 4)  filename_LAKES = inString;
        else if(readcounter == 5)  gid = inString;
        else if(readcounter == 6)  file_DEM = ioFolder + "/" + inString;
        else if(readcounter == 7)  file_TEXT = ioFolder + "/" + inString;
        else if(readcounter == 8)  file_LU = ioFolder + "/" + inString;
        else if(readcounter == 9)  file_VEGETATION = ioFolder + "/" + inString;
        else if(readcounter == 10) file_CHANNEL = ioFolder + "/" + inString;
        else if(readcounter == 11) file_LAKES = ioFolder + "/" + inString;
        else if(readcounter == 12) file_GROUNDWATER = ioFolder + "/" + inString;
        else if(readcounter == 13) file_GROUNDWATERREFERENCE = ioFolder + "/" + inString;
        else if(readcounter == 14) file_DRAINROUTE = ioFolder + "/" + inString;
        else if(readcounter == 15) file_MEASURES = ioFolder + "/" + inString;
        else if(readcounter == 16) scenario = stoi(inString);
        else if(readcounter == 17) riverQ = stoi(inString);
        else if(readcounter == 18) dynamicG = stoi(inString);
        else if(readcounter == 19) erosion = stoi(inString);
        else if(readcounter == 20) gullyThreshold = stoi(inString);
        else if(readcounter == 21) filename_LUPOINTS = inString;
        else if(readcounter == 22) filename_GROUNDWATERCONFIG = inString;
        else if(readcounter == 23) simulateDams = stoi(inString);
        else if(readcounter == 24 && simulateDams == 1)
            filename_DAMS = ioFolder + "/" + inString;
        else if(readcounter == 25 && simulateDams == 1)
            file_DAMS = ioFolder + "/" + inString;
        else if(readcounter == 26)
            filename_HYDROGRAPH = ioFolder + "/" + inString;

        ++readcounter;
    }
}
catch(const std::exception& error)
{
    std::cerr
        << "Invalid value in configuration file near entry "
        << readcounter << ": " << error.what() << std::endl;
    fin.close();
    return 1;
}

fin.close();
fin.clear();

if(readcounter < 26)
{
    std::cerr
        << "Configuration file is incomplete: read "
        << readcounter
        << " key/value entries, expected at least 26."
        << std::endl;
    return 1;
}

std::cout
    << argv[1] << " opened; "
    << readcounter << " configuration entries read."
    << std::endl;
