# ReagentDBClient :crystal_ball:

C++ Client for interacting with ReagentDB. Uses [C++ Rest SDK] and [nlohmann/json].

[C++ Rest SDK]: https://github.com/microsoft/cpprestsdk
[nlohmann/json]: https://github.com/nlohmann/json


Installation
------------
    Place ReagentDBClient.h and ReagentDBClient.lib into your project source folder
    Under Properties > Linker > Input > Additional Dependencies ... Add $(ProjectDir)ReagentDBClient.lib
    Place nlohamann folder from \ReagentDBClient into source folder
    Place cpprest folder from \ReagentDBClient into source folder
    Under Properties > C/C++ > Additional Include Directories ... Add $(ProjectDir)\cpprest
    Place zlib1.dll, libcrypto-1_1.dll, ReagentDBClient.dll into the build folder
    
Usage:

    #include "ReagentDBClient.h"
    
    void sampleFunction() {
      ReagentDBClient db = ReagentDBClient("http://sampleURL.com");
      njson PAList = db.GetPAList();
      ...
    }
    
TODO
------------
Handle interactions with Reagent table

Notes
------------
nlohmann::json has been declared as njson instead, to avoid potential confusion with C++ Rest SDK's web::json class.
 
Thanks
------------
Statically linking C++ Rest SDK via these instructions, adjusting for 32 bit versions:
[Source1](https://stackoverflow.com/questions/44905708/statically-linking-casablanca-cpprest-sdk)
[Source2](https://stackoverflow.com/questions/56097412/how-to-statically-link-cpprest-without-dll-files-in-vs-project/57177759)
