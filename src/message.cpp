#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "sailing.hpp"
#include "message.hpp"
#include "session.hpp"

using namespace std;
using namespace nlohmann;

// should use shared pointer
mymsg_t response_cbor;

struct tm json2date(json& j)
{
    return (struct tm){.tm_hour=j[3], .tm_mday = j[2],.tm_mon = j[1],.tm_year = j[0]};
}

latlon_t json2loc(json& j)
{
    return (latlon_t){j[0], j[1]};
}

json date2json(struct tm& date)
{
    return json({date.tm_year, date.tm_mon, date.tm_mday, date.tm_hour});
}

json loc2json(latlon_t& loc)
{
    return json({loc.lat, loc.lon});
}

std::string datestr(struct tm *date)
{
    char str[30];
    strftime(str, sizeof(str), "%Y-%m-%d %Hhr", date);
    return std::string(str);
}

void printpt(pathpt_t pt)
{
    printf("%s %f,%f\n", datestr(&pt.date).c_str(), pt.loc.lat, pt.loc.lon);
}

json path2json(path_t* path)
{
    json j;
    j["id"]     = path->id;
    j["user"]   = path->user;
    j["startdate"] = date2json(path->startdate);
    j["enddate"] = date2json(path->enddate);
    j["startloc"] = loc2json(path->startloc);
    j["land_collision"] = path->land_collision;

    j["date"] = json::array();
    j["loc"]  = json::array();
    for (int i = 0; i < path->pts.size(); i++)
    {
        j["date"].push_back(date2json(path->pts[i].date));
        j["loc"].push_back(loc2json(path->pts[i].loc));
    }
    return j;
}

path_t json2path(json j)
{
    path_t path;
    path.id = j["id"];
    path.user = j["user"];
    path.startdate = json2date(j["startdate"]);
    path.enddate = json2date(j["enddate"]);
    path.startloc = json2loc(j["startloc"]);
    path.land_collision = j["land_collision"];

    for (json::iterator date = j["date"].begin(), loc = j["loc"].begin(); date != j["date"].end(); ++date, ++loc) {
        pathpt_t pt = {.date = json2date(*date), .loc = json2loc(*loc)};
        //printpt(pt);
        path.pts.push_back(pt);
    }
    return path;
}

mymsg_t msg;

json server_newpath(path_t* path)
{
    return {{"cmd", "new_path"},
            {"path", {
                {"user", path->user},
                {"id", path->id},
                {"startdate", date2json(path->startdate)},
                {"enddate", date2json(path->enddate)},
                {"startloc", loc2json(path->startloc)}
            }}};
}

json server_sendpts(path_t* path, int last_step, int step)
{
    json j = {
        {"cmd", "pts"},
        {"user", path->user},
        {"id", path->id},
        {"step", last_step},
        {"date", json::array()},
        {"loc", json::array()},
    };
    for (int i = last_step; i <= step; i++) {
        j["date"].push_back(date2json(path->pts[i].date));
        j["loc"].push_back(loc2json(path->pts[i].loc));
    }
    return j;
}

int server_decode(my_pss_t *pss, uint8_t *in, size_t len)
{
    //printf("server_decode\n");
    json j;
    try {
        j = json::from_cbor(mymsg_t(in, in + len));
    } catch (...) { return -1; }
    cout << "Message: " << j << "\n";

    if (j["cmd"] == "new_path")
    {
        uint32_t id = Session::new_path();
        path_t* path = &Session::paths[id];
        path->id         = id;
        path->user       = j["user"];
        path->startdate  = json2date(j["startdate"]);
        path->enddate    = json2date(j["enddate"]);
        path->startloc   = json2loc(j["startloc"]);
        path->land_collision = false;

        server_pushall(json::to_cbor(server_newpath(path)));

        int step = 0;
        int last_step = 0;
        path->pts.push_back({path->startdate, path->startloc});
        while(mktime(&path->pts.back().date) < mktime(&path->enddate)) {
            if (sail_step(path) != 0) { printf("Land collision!\n"); path->land_collision = true; path->enddate = path->pts.back().date; }
            else {
                step++;
                if (step%(24*5) == 0) {
                    // printf("id=%d ", path->id); printpt(path->pts.back());
                    // send incrementally
                    server_pushall(json::to_cbor(server_sendpts(path, last_step, step)));
                    last_step = step;
                }
            }
        }
        printf("id=%d ", path->id); printpt(path->pts.back());
        if (last_step < step) {
            // printf("send remaining data %d %d\n", last_step, step);
            server_pushall(json::to_cbor(server_sendpts(path, last_step, step))); // send remaining data here
        }
        if (path->land_collision) server_pushall(json::to_cbor({{"cmd", "land_collision"},{"id", path->id}}));
    }
    else if (j["cmd"] == "restore")
    {
        printf("restore 0x%x\n", pss);

        map<uint32_t, path_t>::iterator it;
        for (it = Session::paths.begin(); it != Session::paths.end(); it++)
        {
            path_t* path = &it->second;
            server_pushto(pss, json::to_cbor(server_newpath(path)));
            server_pushto(pss, json::to_cbor(server_sendpts(path, 0, path->pts.size()-1)));
        }
        server_pushto(pss, json::to_cbor({{"cmd", "ready"}}));
    }
    else if (j["cmd"] == "delete")
    {
        uint32_t id = j["id"];
        uint32_t user = j["user"];
        if (Session::paths[id].user == user)
        {
            Session::paths.erase(id);
            server_pushall(json::to_cbor({{"cmd", "delete"},{"id", id}}));
        }
    }

    return 0;
}