#include <stdio.h>
#include <string.h>


#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>


#include <iostream>
#include "INIReader.h"

#include <curl/curl.h>

#include "libs/base64.h"


#include <iomanip> // for std::setw
#include "json.hpp"

#include "ProgramOptions.hxx"




//Global variables

std::string configFilePath = "../conf/pushauth.conf";

std::string publicKey, privateKey, apiURL;

std::string response_data;
int response_code;

std::string normal_response_data;


std::string mode;
std::string code;
std::string to;
bool response = false;

bool showHashFlag = false;


//Global libs

using json = nlohmann::json;

struct curl_fetch_st {
    char *payload;
    size_t size;
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *) userp)->append((char *) contents, size * nmemb);
    return size * nmemb;
}

bool setKeys() {
    INIReader reader(configFilePath);

    if (reader.ParseError() < 0) {
        return false;
    }

    //std::cout << "Res body: " << reader.Get("keys", "public_key", "UNKNOWN") << "\n\r";


    publicKey = reader.Get("keys", "public_key", "UNKNOWN");
    privateKey = reader.Get("keys", "private_key", "UNKNOWN");

    apiURL = reader.Get("api", "url", "https://api.pushauth.io");

    return true;
}

bool check_signature(std::string data, std::string hmac) {

    std::string key = privateKey;

    //std::cout << "private key: " << key << "\n\r";


    unsigned int diglen;

    unsigned char resulter[EVP_MAX_MD_SIZE];

    HMAC(EVP_sha256(),
         reinterpret_cast<const unsigned char *>(key.c_str()), key.length(),
         reinterpret_cast<const unsigned char *>(data.c_str()), data.length(),
         resulter, &diglen);

    std::string hmac_data = base64_encode(resulter, diglen);

    if (hmac_data == hmac) {
        return true;
    } else {
        return false;
    }

}

std::string encode_hmac_base64(std::string sjson) {

    unsigned int diglen;

    unsigned char resulter[EVP_MAX_MD_SIZE];

    //std::cout << "input hmac data: " << sjson << "\n\r";

    //std::string encoded_data = base64_encode(reinterpret_cast<const unsigned char*>(sjson.c_str()), sjson.length());

    HMAC(EVP_sha256(), reinterpret_cast<const unsigned char *>(privateKey.c_str()), privateKey.length(),
         reinterpret_cast<const unsigned char *>(sjson.c_str()), sjson.length(),
         resulter, &diglen);

    std::string hmac_data = base64_encode(resulter, diglen);

    return hmac_data;

}

bool request(std::string req_body, std::string type) {

    std::string path;


    CURL *curl;
    CURLcode res;

    std::string readBuffer;
    struct curl_slist *headers = NULL;

    struct curl_fetch_st curl_fetch;                        /* curl fetch struct */
    struct curl_fetch_st *cf = &curl_fetch;                 /* pointer to fetch struct */



    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl = curl_easy_init();

    if ((curl = curl_easy_init()) == NULL) {


        //req_error = "ERROR: Failed to create curl handle in fetch_session";
        fprintf(stderr, "ERROR: Failed to create curl handle in fetch_session");

        return false;
    }

    if (type == "status") {
        path = "/push/status";
        //path = "/test";
    } else if (type == "request") {
        path = "/push/send";
    } else if (type == "qr") {
        path = "/qr/show";
    }

    //const char charPath = reinterpret_cast<const char>(path.c_str());

    std::string fullURL = apiURL + path;

    char *url = reinterpret_cast<char *>(const_cast<char *>(fullURL.c_str()));

    //std::cout << url;

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 45);
    //curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 1);  // for --insecure option
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req_body.length());
    curl_easy_setopt(curl, CURLOPT_POST, 1);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);


    if (res != CURLE_OK || cf->size < 1) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s \n\r",
                url, curl_easy_strerror(res));
        /* return error */
        return false;
    }

    //CURLcode ret;
    long http_code = 0;

    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_ABORTED_BY_CALLBACK) {


        auto j_res = json::parse(readBuffer);
        //j_res["code"]=http_code;

        response_code = http_code;
        response_data = j_res.dump();

        return true;
    }

}


bool decode_response_data(std::string data) {

    auto j_response = json::parse(data);
    if (response_code == 200) {


        std::string res_body, res_hmac;
        std::string res_body_data = j_response["data"];

        std::istringstream iss(res_body_data);

        std::getline(iss, res_body, '.');
        std::getline(iss, res_hmac, '.');

        if (check_signature(res_body, res_hmac) == false) {

            std::cerr << "Signature not correct!";
            return false;

        }

        normal_response_data = base64_decode(res_body);

        return true;


    } else {
        normal_response_data = response_data;
        return true;
    }

}


bool show_req_status(std::string req_hash) {

    json j_data;
    json j_body;

    std::string reqType = "status";


    j_data["req_hash"] = req_hash;
    std::string sjson = j_data.dump();

    std::string encoded_data = base64_encode(reinterpret_cast<const unsigned char *>(sjson.c_str()), sjson.length());

    j_body["pk"] = publicKey;
    j_body["data"] = encoded_data + '.' + encode_hmac_base64(encoded_data);

    std::string req_body = j_body.dump();


    if (request(req_body, reqType) == true) {

        // auto j_response = json::parse(response_data);

        if (decode_response_data(response_data) == true) {
            return true;
        } else {
            return false;
        }


    } else {
        return false;
    }


}


bool show_qr_body() {

    json j_data;
    json j_body;

    std::string reqType = "qr";


    j_data["image"]["size"] = "128";
    j_data["image"]["color"] = "40,0,40";
    j_data["image"]["backgroundColor"] = "255,255,255";
    j_data["image"]["margin"] = "1";


    std::string sjson = j_data.dump();

    std::string encoded_data = base64_encode(reinterpret_cast<const unsigned char *>(sjson.c_str()), sjson.length());

    j_body["pk"] = publicKey;
    j_body["data"] = encoded_data + '.' + encode_hmac_base64(encoded_data);

    std::string req_body = j_body.dump();


    if (request(req_body, reqType) == true) {

        // auto j_response = json::parse(response_data);

        if (decode_response_data(response_data) == true) {
            return true;
        } else {
            return false;
        }


    } else {
        return false;
    }


}

void show_response_qr() {

    //std::cout << normal_response_data << std::endl;

    if (response_code == 200) {
        auto j_response_clear = json::parse(normal_response_data);

        if (showHashFlag == true) {
            std::cout << "Request hash: " << j_response_clear["req_hash"] << std::endl;
        }

        std::cout << "QR-image URL: " << j_response_clear["qr_url"] << std::endl;


    } else {
        auto j_response_clear = json::parse(normal_response_data);

        std::cerr << "Server return error." << std::endl;

        std::cerr << "Code: " << j_response_clear["status_code"] << std::endl;

        std::cerr << "Message: " << j_response_clear["message"] << "\n\r";
    }


}


bool push_request() {

    json j_data;
    json j_body;

    std::string reqType = "request";

    j_data["addr_to"] = to;
    j_data["mode"] = mode;
    j_data["code"] = code;
    j_data["flash_response"] = response;

    std::string sjson = j_data.dump();


    std::string encoded_data = base64_encode(reinterpret_cast<const unsigned char *>(sjson.c_str()), sjson.length());

    j_body["pk"] = publicKey;
    j_body["data"] = encoded_data + '.' + encode_hmac_base64(encoded_data);

    std::string req_body = j_body.dump();

    if (request(req_body, reqType) == true) {

        // auto j_response = json::parse(response_data);

        if (decode_response_data(response_data) == true) {
            return true;
        } else {
            return false;
        }


    } else {
        return false;
    }

}

void show_response_result() {

    //std::cout << normal_response_data << std::endl;

    if (response_code == 200) {
        auto j_response_clear = json::parse(normal_response_data);


        if (response == true) {

            std::cout << "Request hash: " << j_response_clear["req_hash"] << std::endl;

            return;

        }

        if (showHashFlag == true) {
            std::cout << "Request hash: " << j_response_clear["req_hash"] << std::endl;
        }


        if (j_response_clear["answer"] == true) {
            std::cout << "Answer: " << "true" << std::endl;
            //return 0;
        } else if (j_response_clear["answer"] == false) {
            std::cout << "Answer: " << "false" << std::endl;
            //return 0;
        } else if (j_response_clear["answer"].is_null()) {
            std::cout << "Answer: " << "timeout" << std::endl;
            //return 0;
        }
    } else {
        auto j_response_clear = json::parse(normal_response_data);

        std::cerr << "Server return error." << std::endl;

        std::cerr << "Code: " << j_response_clear["status_code"] << std::endl;

        std::cerr << "Message: " << j_response_clear["message"] << "\n\r";
    }


}


int main(int argc, char *argv[]) {


    po::parser parser;


    parser["mode"]
            .abbreviation('m')
            .description("type of request: push, code or qr")
            .type(po::string)
            .multi();
    parser["code"]
            .abbreviation('c')
            .description("code value only for --mode=code")
            .type(po::string)
            .multi();
    parser["to"]
            .abbreviation('t')
            .description("email address of client")
            .type(po::string)
            .multi();
    parser["response"]
            .abbreviation('r')
            .description("waiting for client answer. Ignoring for --mode=code")
            .type(po::string)
            .multi();
    parser["status"]
            .abbreviation('s')
            .description("get remote answer status by request hash")
            .type(po::string)
            .multi();
    parser["hash"]
            .abbreviation('v')
            .description("show request hash")
            .type(po::string)
            .multi();
    parser["config"]
            .abbreviation('i')
            .description("path to config file")
            .type(po::string)
            .multi();

    parser["help"]            // corresponds to --help
            .abbreviation('?')    // corresponds to -?
            .description("print this help screen")
            .callback([&] { std::cout << parser << '\n'; });
    // callbacks get invoked when the option occurs


    if (!parser(argc, argv)) {
        std::cerr << "errors occurred; aborting\n";
        return -1;
    }
    // we don't want to print anything else if the help screen has been displayed
    if (parser["help"].size())
        return 0;


    auto &&m = parser["mode"];
    auto &&c = parser["code"];
    auto &&t = parser["to"];
    auto &&r = parser["response"];
    auto &&s = parser["status"];

    auto &&v = parser["hash"];

    auto &&i = parser["config"];

    //If config file set or use default
    if (i.was_set()) {
        configFilePath = i.get().string;
    }

    //Setting keys
    if (setKeys() == false) {
        std::cerr << "Can not load config file\n";
        return -1;
    }

    //If config file set or use default
    if (v.was_set()) {
        showHashFlag = true;
    }



    //Catch if request status
    if (s.was_set()) {
        if (show_req_status(s.get().string) == false) {
            return -1;
        } else {
            show_response_result();
            return 0;
        }


    }


    if (m.was_set() == false) {
        std::cerr << "Mode not setting.\n\r" << "Run program with key -? for more information." << std::endl;
        return -1;
    }

    if (m.get().string == "push") {
        mode = "push";
    } else if (m.get().string == "code") {
        mode = "code";

        if (c.was_set()) {
            code = c.get().string;
        } else {
            std::cerr << "Code not setting.\n\r" << "Run program with key -? for more information." << std::endl;
            return -1;
        }

    } else if (m.get().string == "qr") {
        mode = "qr";


    } else {
        std::cerr << "Mode not correct.\n\r" << "Run program with key -? for more information." << std::endl;
        return -1;
    }


    //Catch QR-code
    if (mode == "qr") {

        if (show_qr_body() == false) {
            return -1;
        } else {
            show_response_qr();
            return 0;
        }

    }


    if (t.was_set()) {
        to = t.get().string;
    } else {
        std::cerr << "Address not setting.\n\r" << "Run program with key -? for more information." << std::endl;
        return -1;
    }

    if (r.was_set()) {
        response = true;
    } else {
        response = false;
    }

    if (push_request() == false) {
        return -1;
    } else {
        show_response_result();
        return 0;
    }


}






