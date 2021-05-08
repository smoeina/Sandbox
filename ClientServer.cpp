#include "crow.h"
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <ShellAPI.h>
#include <boost/format.hpp>
using namespace std;

vector<string> msgs;

vector<pair<crow::response*, decltype(chrono::steady_clock::now())>> ress;

void broadcast(const string& msg)
{
    msgs.push_back(msg);
    crow::json::wvalue x;
    x["msgs"][0] = msgs.back();
    x["last"] = msgs.size();
    string body = crow::json::dump(x);
    for (auto p : ress)
    {
        auto* res = p.first;
        CROW_LOG_DEBUG << res << " replied: " << body;
        res->end(body);
    }
    ress.clear();
}

void PrintMemoryInfo(DWORD processID)
{
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;
    vector<string> processes_info;

    // Print the process identifier.

    printf("\nProcess ID: %u\n", processID);

    // Print information about the memory usage of the process.

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, processID);
    if (NULL == hProcess)
        return;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
    {
        printf("\tPageFaultCount: 0x%08X\n", pmc.PageFaultCount);
        /*processes_info.push_back("\tPageFaultCount: "+ pmc.PageFaultCount);
        processes_info.push_back("\PeakWorkingSetSize: " + pmc.PeakWorkingSetSize);
        processes_info.push_back("\QuotaPeakPagedPoolUsage: " + pmc.QuotaPeakPagedPoolUsage);
        processes_info.push_back("\QuotaPagedPoolUsage: " + pmc.QuotaPagedPoolUsage);
        processes_info.push_back("\QuotaPeakNonPagedPoolUsage: " + pmc.QuotaPeakNonPagedPoolUsage);
        processes_info.push_back("\PagefileUsage: " + pmc.PagefileUsage);
        processes_info.push_back("\PeakPagefileUsage: " + pmc.PeakPagefileUsage);*/

        printf("\tPeakWorkingSetSize: 0x%08X\n",
            pmc.PeakWorkingSetSize);

        printf("\tWorkingSetSize: 0x%08X\n", pmc.WorkingSetSize);
        printf("\tQuotaPeakPagedPoolUsage: 0x%08X\n",
            pmc.QuotaPeakPagedPoolUsage);
        printf("\tQuotaPagedPoolUsage: 0x%08X\n",
            pmc.QuotaPagedPoolUsage);
        printf("\tQuotaPeakNonPagedPoolUsage: 0x%08X\n",
            pmc.QuotaPeakNonPagedPoolUsage);
        printf("\tQuotaNonPagedPoolUsage: 0x%08X\n",
            pmc.QuotaNonPagedPoolUsage);
        printf("\tPagefileUsage: 0x%08X\n", pmc.PagefileUsage);
        printf("\tPeakPagefileUsage: 0x%08X\n",
            pmc.PeakPagefileUsage);


        for (int i = 0; i < processes_info.size(); i++)
            std::cout << processes_info[i] << ' ';
    }

    CloseHandle(hProcess);
}


VOID startup(LPCTSTR lpApplicationName)
{
    // additional information
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    CreateProcess(lpApplicationName,   // the path
        NULL,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}



int main()
{
    crow::SimpleApp app;
    crow::mustache::set_base(".");

    CROW_ROUTE(app, "/")
        ([] {
        crow::mustache::context ctx;
        return crow::mustache::load("example_chat.html").render();
            });

    CROW_ROUTE(app, "/logs")
        ([] {
        DWORD aProcesses[1024], cbNeeded, cProcesses;
        unsigned int i;

        if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        {
            return 1;
        }

        // Calculate how many process identifiers were returned.

        cProcesses = cbNeeded / sizeof(DWORD);

        // Print the memory usage for each process

        for (i = 0; i < cProcesses; i++)
        {
            PrintMemoryInfo(aProcesses[i]);
        }
        return 2;
            });



    CROW_ROUTE(app, "/send_file")
        .methods("GET"_method, "POST"_method)
        ([](const crow::request& req)
            {
               CROW_LOG_INFO << "msg from client: " << req.body;
                auto itr = req.headers.find("file_name");
                string file_name = (&itr._Ptr->_Myval)->second;
                ofstream MyFile(file_name, std::ios_base::binary);
                MyFile << req.body;
                CROW_LOG_INFO << "File with name: " << file_name << " Recevied...";
                MyFile.flush();
                MyFile.close();
                return "";
            });

    CROW_ROUTE(app, "/execute_app")
        .methods("GET"_method, "POST"_method)
        ([](const crow::request& req)
            {
                CROW_LOG_INFO << "msg from client: " << req.body;
                auto itr = req.headers.find("file_name");
                string file_name = (&itr._Ptr->_Myval)->second;
                std::wstring temp = wstring(file_name.begin(), file_name.end());
                LPCWSTR lpcwstr = temp.c_str();
                CROW_LOG_INFO << "Application Runned Successfully...";
                startup(lpcwstr);
                return "Application Executed Successfully...";

            });

    app.port(8304)
       .multithreaded()
        .run();
}