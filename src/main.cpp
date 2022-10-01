//==========================================================================================================
// smartlynq_static_ip - Assigns a static IP address to a Xilinx SmartLynq JTAG programmer
//
// Author: Doug Wolf
//
//
// Top-level program flow is in the "execute()" routine.
//==========================================================================================================
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <map>
#include <filesystem>
#include "config_file.h"
#include "history.h"

using namespace std;

// We're going to use a lot of these, so make it convenient
typedef vector<string> strvec;

// This is the symbol table that we'll use for text substitutions
map<string,string> symbolTable;

// The vivado script that we'll run to program the IP address
strvec vivadoScript;

// The contents of the config.ini file that we use during IP address programming
strvec configIni;

// The fully qualified path to the Vivado executable
string vivado;

// This is the Vivado command line we'll execute
string vivadoCommandLine;

// Name of a directory where we can store temporary files
string tmp;

// This is all of the symbols we support
const string USB_IP       = "%usb_ip%";
const string STATIC_IP    = "%static_ip%";
const string GATEWAY_IP   = "%gateway_ip%";
const string VIVADO       = "%vivado%";
const string TMP          = "%tmp%";

// Function prototypes
void   execute(int argc, const char** argv);
void   parseCommandLine(int argc, const char** argv);
void   showHelp();
void   computeGatewayIP();
void   execute();
void   readConfigurationFile();
string translate(const string&);
void   translate(strvec&);
void   writeStringsToFile(strvec&, string filename);
strvec shell(const char* fmt, ...);
int    runVivado();

//==========================================================================================================
// main() - Runs the program and if an exception is thrown, displays the error and exits
//==========================================================================================================
int main(int argc, const char** argv)
{
    try
    {
        execute(argc, argv);
    }
    catch(const std::exception& e)
    {
        cerr << e.what() << '\n';
    }
}
//==========================================================================================================



//==========================================================================================================
// execute() - Contains the top-level program
//==========================================================================================================
void execute(int argc, const char** argv)
{
    // Parse the command line
    parseCommandLine(argc, argv);

    // Compute the IP address of the gateway
    computeGatewayIP();

    // Read in the configuration file
    readConfigurationFile();

    // Perform macro substitution on the Vivado command line
    vivadoCommandLine = translate(vivadoCommandLine);

    // Perform macro substituion on the contents of the 'config.ini' file
    translate(configIni);

    // Perform macro substitution on the contents of the Vivado script
    translate(vivadoScript);

    // Write the 'config.ini' file to disk
    writeStringsToFile(configIni, tmp+"/config.ini");
    
    // Write the Vivado script to disk
    writeStringsToFile(vivadoScript, tmp+"/script.tcl");

    // Run Vivado to do the actual programming of the static IP address
    int rc = runVivado();

    // Tell the OS whether or not we succeded
    exit(rc);
}
//==========================================================================================================


//==========================================================================================================
// parseCommandLine() - Fetches the USB IP address and desired static IP address from the command line
//
// Passed: argc = Number of command line parameters (including the name of the executable)
//         argv = Array of pointers to the command line parameters
//
// On Exit: symbolTable[USB_IP]    = The current USB IP address of the SmartLynq JTAG programmer
//          symbolTable[STATIC_IP] = The static IP address to be programmed into the SmartLynq
//==========================================================================================================
void parseCommandLine(int argc, const char** argv)
{
    uint32_t ip;

    // There should be exactly two parameters on the command line
    if (argc != 3) showHelp();

    // Ensure that the USB IP address is a properly formatted IPv4 address
    if (inet_pton(AF_INET, argv[1], &ip) < 1)
    {
        cerr << argv[1] << " is malformed\n";
        exit(1);
    }

    // Ensure that the static IP address is a properly formatted IPv4 address
    if (inet_pton(AF_INET, argv[2], &ip) < 1)
    {
        cerr << argv[2] << " is malformed\n";
        exit(1);
    }

    // Save the two IP address into the symbol table
    symbolTable[USB_IP]    = argv[1];
    symbolTable[STATIC_IP] = argv[2];
}
//==========================================================================================================




//==========================================================================================================
// computeGatewayIP() - Compute the IP address of the gateway that will be programmed into SmartLynq
//
// On Entry: symbolTable[STATIC_IP] = The static IP to be programmed
//
// On Exit:  symbolTable[GATEWAY_IP] = The gateway IP address to be programmed
//==========================================================================================================
void computeGatewayIP()
{   
    unsigned char octet[4];
    char buffer[50];

    // Fetch the static IP address
    string ip = symbolTable[STATIC_IP];

    // Convert the static IP address from a string into the four octets
    inet_pton(AF_INET, ip.c_str(), octet);

    // Change the last octet to a 1.  For instance 10.11.12.3 becomes 10.11.12.1
    octet[3] = 1;

    // Covert the octets back to a dotted quad IP address
    inet_ntop(AF_INET, octet, buffer, sizeof buffer);

    // And store the result in our symbol table
    symbolTable[GATEWAY_IP] = buffer;
}
//==========================================================================================================




//==========================================================================================================
// showHelp() - Displays usage text, then exits
//==========================================================================================================
void showHelp()
{
    cout << "Version " SW_VERSION "\n";
    printf("Usage: smartlynq_static_ip <USB_IP_ADDRESS> <STATIC_IP_ADDRESS>\n");
    exit(1);    
}
//==========================================================================================================



//==========================================================================================================
// readConfigurationFile() - Reads in the configuration specifications
//==========================================================================================================
void readConfigurationFile()
{
    CConfigFile cf;

    // This is the name of the file that contains our configuration
    string filename = "smartlynq_static_ip.conf";

    // Read in the config file
    if (!cf.read(filename, false)) throw runtime_error("Can't open "+filename);

    // Fetch the name and path of the vivado executable
    cf.get("vivado", &vivado);
    symbolTable[VIVADO] = vivado;

    // Fetch the name of the temp directory
    cf.get("tmp", &tmp);
    symbolTable[TMP] = tmp;

    // Fetch the Vivado command line that we'll execute
    cf.get("command_line", &vivadoCommandLine);

    // Fetch the script that will be stored in the config.ini file
    cf.get_script_vector("config.ini", &configIni);

    // Fetch the script that will be run by Vivado 
    cf.get_script_vector("vivado_script", &vivadoScript);
}
//==========================================================================================================


//==========================================================================================================
// translate() - Uses the symbolTable to perform text substitution in a string
//==========================================================================================================
string translate(const string& raw)
{
    string result = raw;

    // Loop through every key-value pair in the symbol table
    for (auto& pair : symbolTable)
    {
        // Get the key/value pair for this symbol-table entry
        const string& key   = pair.first;
        const string& value = pair.second;

        // Search for every occurence of this key, and subsitute the value in it's place        
        while (true)
        {
            auto pos = result.find(key);
            if (pos == string::npos) break;
            result.replace(pos, key.length(), value);
        }
    }

    // Return the fully translated string to the caller
    return result;
}
//==========================================================================================================



//==========================================================================================================
// translate() - Uses the symbolTable to perform text substitution of every string in a vector
//==========================================================================================================
void translate(strvec& v)
{
    for (auto& s : v) s = translate(s);
}
//==========================================================================================================



//==========================================================================================================
// writeStringsToFile() - Writes every string in a string-vector to the specified filename
//==========================================================================================================
void writeStringsToFile(strvec& v, string filename)
{
    // Open the file
    ofstream ofile(filename);

    // If we can't open the file, that's fatal
    if (!ofile.is_open()) throw runtime_error("Can't create " + filename);

    // Write every string in vector to the file
    for (auto& s : v) ofile << s << "\n";

    // And we're done writing the file
    ofile.close();
}
//==========================================================================================================


//==========================================================================================================
// chomp() - Removes any carriage-return or linefeed from the end of a buffer
//==========================================================================================================
static void chomp(char* buffer)
{
    char* p;
    p = strchr(buffer, 10);
    if (p) *p = 0;
    p = strchr(buffer, 13);
    if (p) *p = 0;
}
//==========================================================================================================


//==========================================================================================================
// shell() - Executes a shell command and returns the output as a vector of strings
//==========================================================================================================
strvec shell(const char* fmt, ...)
{
    strvec result;
    va_list        ap;
    char           command[1024];
    char           buffer[1024];

    // Format the command
    va_start(ap, fmt);
    vsprintf(command, fmt, ap);
    va_end(ap);

    // Run the command
    FILE* fp = popen(command, "r");

    // If we couldn't do that, give up
    if (fp == nullptr) return result;

    // Fetch every line of the output and push it to our vector of strings
    while (fgets(buffer, sizeof buffer, fp))
    {
        chomp(buffer);
        result.push_back(buffer);
    }

    // When the program finishes, close the FILE*
    fclose(fp);

    // And hand the output of the program (1 string per line) to the caller
    return result;
}
//==========================================================================================================



//==========================================================================================================
// runVivado() - Uses the Vivado TCL scripting engine to program the static IP address into the SmartLynq
//==========================================================================================================
int runVivado()
{
    strvec result;

    // Assume for the moment that we will succeed
    bool failed = false;

    // Run "%vivado% -help", just to find out if Vivado exists and is runnable
    result = shell("%s -help 2>&1", vivado.c_str());

    // If the output of that command is just one line, Vivado doesn't exist
    if (result.size() < 2) throw runtime_error("Vivado not found!");

    // This will take a moment, so make sure the user knows what we're doing
    cout << "Programming static IP " << symbolTable[STATIC_IP] << "\n";
    
    // Run Vivado, capturing it's output into "result"
    result = shell("%s", vivadoCommandLine.c_str());

    // Save the Vivado output to a file just for debugging purposes
    writeStringsToFile(result, tmp+"/script.result");

    // If the result vector is very short, it means Vivado couldn't be found
    if (result.size() < 2) throw runtime_error("Vivado not found");

    // Loop through each line of the Vivado output
    for (auto& s : result)
    {
        // Extract the first word from the line
        std::string firstWord = s.substr(0, s.find(" "));

        // If the first word is "ERROR:" or "couldn't", Vivado is reporting that something has gone awry
        if (firstWord == "ERROR:" || firstWord == "couldn't")
        {
            failed = true;
            break;
        }
    }

    // If we failed, show the Vivado output to the user
    if (failed)
    {
        cout << "FAILED!!  Vivado says:\n";
        for (auto& s : result) cout << s << "\n";
        return 1;       
    }

    // If we get here, we've succeded
    cout << "Success!\n";

    // Tell the caller that no error occured
    return 0;
}
//==========================================================================================================
