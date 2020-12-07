

#include <iostream>
#include <uwebsockets/App.h>
#include <regex>
#include <map>
#include <stdlib.h>
#include <string>
#include <algorithm>
using namespace std;

struct UserConnection {
    string name;
    unsigned long user_id;

};

void newUserConnected(string channel, uWS::WebSocket<false,true>* ws, string name, unsigned long user_id) {
    string_view message("NEW_USER=" + to_string(user_id) + "," + name);
    ws->publish(channel, message);
}
int main()
{
    int port = 8888;
    unsigned long latest_user_id = 10;
    map<unsigned long, string> online_users = {{ 1, "BOT" }};

    map<string, string> database = {
        {"hello", "Oh, hello to you human"},
        {"how are you", "I am good"},
        {"what are you doing", "I am answering stupid question"},
        {"what is your name", "My name is Skill Bot 3000"},
        {"bye", "Ok, byeee"}
    };

    uWS::App().ws<UserConnection>("/*", {
        // settings
        .open = [&latest_user_id, &online_users](auto* ws) {
            UserConnection* data = (UserConnection*)ws->getUserData();
            data->user_id = latest_user_id++;
            data->name = "UNNAMED";
            cout << "New user connected ID =" << data->user_id << endl;
            string user_channel("user#" + to_string(data->user_id));
            ws->subscribe(user_channel);
            ws->subscribe("broadcast");

            for (auto entry : online_users) {
                newUserConnected(user_channel, ws, entry.second, entry.first);
            }
            online_users[data->user_id] = data->name;
            cout << "Total number of users connected now is " << online_users.size() << endl;

        },
        .message = [&online_users, &database](auto ws, string_view message, uWS::OpCode opCode) {
            string SET_NAME("SET_NAME=");
            string MESSAGE_TO("MESSAGE_TO=");

            UserConnection* data = (UserConnection*)ws->getUserData();
            cout << "New message received = " << message << endl;
            if (message.find("SET_NAME=") == 0) {
                auto rest_name = message.substr(SET_NAME.length());
                if (rest_name.find(",") == string::npos && rest_name.length() < 256) {
                    cout << "User sets their name as " << rest_name << endl;
                    data->name = rest_name;
                    online_users[data->user_id] = data->name;
                    newUserConnected("broadcast",ws, data->name, data->user_id);
                }
                else {
                    cout << "Syntax error, try again";
                }
            }
            if (message.find("MESSAGE_TO=") == 0) {
                cout << "User sends private message" << endl;
                auto rest = string(message.substr(MESSAGE_TO.length())); //ID, text
                int comma_position = rest.find(",");
                auto ID = rest.substr(0, comma_position);
                auto text = data->name + ": " + rest.substr(comma_position + 1);
                if (online_users.count(strtoull(ID.c_str(),NULL,10)) == 0) {
                    cout << "Error, there is no user with ID = " << string(ID) << endl;
                }
                else {
                    ws->publish("user#" + string(ID), text);
                    ws->publish("user#" + to_string(data->user_id), "ME: " + text);
                    cout << "Message was sent successfully" << endl;
                }
                if (ID == "1") {
                    ws->publish("user#" + to_string(data->user_id), "BOT is typing..");
                    bool isAnswerFound = false;
                    for (auto entry : database) {
                        regex pattern = regex(".*" + entry.first + ".*");
                        transform(text.begin(), text.end(), text.begin(), ::tolower);
                        if (regex_match(text, pattern)) {
                            ws->publish("user#" + to_string(data->user_id), "BOT: " + entry.second);
                            isAnswerFound = true;
                        }
                    }
                    if (!isAnswerFound) {
                        ws->publish("user#" + to_string(data->user_id), "BOT: Sorry, I do not understand\n");
                    }
                }
                // Include sender name
            }
        },
        .close = [&online_users](auto* ws, int code, std::string_view message) {
            UserConnection* data = (UserConnection*)ws->getUserData();
            ws->publish("broadcast", "DISCONNECT= USER " + data->name + " with ID " + to_string(data->user_id));
            online_users.erase(data->user_id);
        }
        }).listen(port, [port](auto* token) {
            if (token) {
                cout << "Server started successfully on port " << port << endl;
            }
            else {
                cout << "Server failed to start" << endl;
            }
        }).run();

}



