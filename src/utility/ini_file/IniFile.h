#pragma once

#include <string>
#include <map>
using namespace std;

class Value
{
private:
    /* data */
public:
    Value(/* args */);
    Value(bool value);
    Value(int value);
    Value(double value);
    Value(const char* value);
    Value(const string& value);

    Value& operator = (bool value);
    Value& operator = (int value);
    Value& operator = (double value);
    Value& operator = (const char* value);
    Value& operator = (const string& value);

    operator bool();
    operator int();
    operator double();
    operator string();

private:
    string m_value;
};

typedef std::map<string, Value> Section;

class IniFile
{
public:
    IniFile( );
    bool load(const string& filename);
    bool save(const string& filename);
    void show();
    string str();

    Value& get(const string& section, const string &key);
    void set (const string& section, const string& key, const Value& value);
    bool has(const string& section, const string& key);
    bool has(const string& section);
    void remove(const string& section, const string& key);
    void remove(const string& section);
    void clear();

    Section& operator [] (const string& section)
    {
        return m_sections[section];
    }

private:
    string trim(string s);

private:
    string m_filename;
    std::map<string, Section> m_sections;
};
