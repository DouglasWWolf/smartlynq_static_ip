//==========================================================================================================
// config_file.h - Defines a parser for configuration/settings files
//==========================================================================================================
#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>

//----------------------------------------------------------------------------------------------------------
// CConfigScript() - Provides a convenient interface for parsing script-specs in a config-file
//----------------------------------------------------------------------------------------------------------
class CConfigScript
{
public:

    // After reset "get_next_line()" fetches the first line of the script
    void        rewind() {m_line_index = 0;}

    // Call this to begin processing the next line of the script
    bool        get_next_line(int *p_token_count = NULL, std::string *p_text = NULL);

    std::string get_next_token(bool make_lower = false);
    int32_t     get_next_int();
    double      get_next_float();

    // Call this to erase the script
    void        make_empty();

    // Overloading the '=' operator so we can assign a string vector
    void        operator=(const std::vector<std::string> rhs) {m_script = rhs; rewind();}

protected:

    // This is index of the next line to be fetched via "get_next_line()"
    int         m_line_index;

    // This is the index of the next token to be fetched
    int         m_token_index;

    // These are the lines of the script
    std::vector<std::string> m_script, m_tokens;
};
//----------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------
// CConfigFile - Provides a convenient interface for reading configuration files
//----------------------------------------------------------------------------------------------------------
class CConfigFile
{

public:
    
    // Default constructor
    CConfigFile() {m_throw_on_fail = true;}

    // Call this to read the config file.  Returns 'true' on success, 'false' if file not found
    bool    read(std::string filename, bool msg_on_fail = true);

    // Call this to set the name of section to use for name scoping
    void    set_current_section(std::string section);

    // Call this to determine whether an exception is thrown when trying to fetch an unknown key
    void    throw_on_fail(bool flag = true) {m_throw_on_fail = flag;}

    // Call this to fetch a variable-type configuration spec.
    // Can throw exception runtime_error
    bool    get(std::string key, std::string fmt, void* p1=NULL, void* p2=NULL, void* p3=NULL
                                                , void* p4=NULL, void* p5=NULL, void* p6=NULL
                                                , void* p7=NULL, void* p8=NULL, void* p9=NULL);

    // Call this to fetch integers.
    // Can throw exception runtime_error
    bool    get(std::string key, int32_t* p1=NULL, int32_t* p2=NULL, int32_t* p3=NULL
                               , int32_t* p4=NULL, int32_t* p5=NULL, int32_t* p6=NULL
                               , int32_t* p7=NULL, int32_t* p8=NULL, int32_t* p9=NULL);

    // Call this to fetch doubles
    // Can throw exception runtime_error
    bool    get(std::string key, double* p1=NULL, double* p2=NULL, double* p3=NULL
                               , double* p4=NULL, double* p5=NULL, double* p6=NULL
                               , double* p7=NULL, double* p8=NULL, double* p9=NULL);

    // Call this to fetch stringss
    // Can throw exception runtime_error
    bool    get(std::string key, std::string* p1=NULL, std::string* p2=NULL, std::string* p3=NULL
                               , std::string* p4=NULL, std::string* p5=NULL, std::string* p6=NULL
                               , std::string* p7=NULL, std::string* p8=NULL, std::string* p9=NULL);

    // Call this to fetch bools
    // Can throw exception runtime_error
    bool    get(std::string key, bool* p1=NULL, bool* p2=NULL, bool* p3=NULL
                               , bool* p4=NULL, bool* p5=NULL, bool* p6=NULL
                               , bool* p7=NULL, bool* p8=NULL, bool* p9=NULL);

    // Call these to fetch a vector of values
    // Can throw exception runtime_error
    bool    get(std::string, std::vector<int32_t    > *p_values);
    bool    get(std::string, std::vector<double     > *p_values);
    bool    get(std::string, std::vector<std::string> *p_values);
    bool    get(std::string, std::vector<bool       > *p_values);
    

    // Call this to fetch a script-spec from the config file    
    bool    get(std::string, CConfigScript* p_script);

    // Call this to fetch a script spec as a vector of string
    bool    get_script_vector(std::string, std::vector<std::string>*);

    // Tells the caller whether or not the specified spec-name exists
    bool    exists(std::string key) {return exists(key, NULL);}

    // Dumps out the m_specs in a human-readable form.  This is strictly for testing
    void    dump_specs();

protected:

    // If this is true, fetching the value of an unknown spec will throw 
    bool    m_throw_on_fail;

    // A strvec_t is a vector of strings
    typedef std::vector< std::string > strvec_t;

    // Call this to fetch the values-vector associated with a key.  Can throw exception!
    bool    lookup(std::string key, strvec_t *p_result);

    // Call this to fetch the values-vector associated with a key.  Won't throw excption
    bool    exists(std::string, strvec_t *p_result);

    // The section name to look for specs in
    std::string m_current_section;

    // Our configuration specs are a map of string vectors
    std::map<std::string, strvec_t> m_specs;
};
//----------------------------------------------------------------------------------------------------------




