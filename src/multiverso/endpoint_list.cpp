#include <fstream>
#include "log.h"
#include "endpoint_list.h"

namespace multiverso
{
    EndpointList::EndpointList(std::string filename)
    {
        int id;
        char str[64];
        std::vector<int> ids;
        std::vector<std::string> endpoints;

        FILE *file = fopen(filename.c_str(), "r");
        if (file == nullptr)
        {
            Log::Error("Error on creating EndpointList, FILE OPENING FAIL: %s\n",
                filename.c_str());
            return;
        }
        while (fscanf(file, "%d %s", &id, &str) > 0)
        {
            ids.push_back(id);
            endpoints.push_back(str);
        }
        fclose(file);

        endpoints_.resize(ids.size());
        for (int i = 0; i < ids.size(); ++i)
        {
            endpoints_[i] = endpoints[i];
        }
    }

    EndpointList::~EndpointList() {}

    std::string EndpointList::GetEndpoint(int id)
    {
        return (0 <= id && id < endpoints_.size()) ? endpoints_[id] : "";
    }
}
