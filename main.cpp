 /***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
 
//Send e-mail with SMTP

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <fstream>
#include <string>
#include <sstream>

/*
    The libcurl options want plain address, the viewable headers in the mail 
    can very well get a full name as well.
 */

//Replace values with the email the program will use and its password or passkey.
#define FROM_ADDR "sender@email.com"
#define FROM_PASS "password1"
#define SMTP_SERVER "smtp://smtp.gmail.com:587"
//I am using smtp://smtp.gmail.com:587 but replace with smtp server of choice

//Filenames for the email content and receiver list.
#define PAYLOAD "payload.txt"
#define RECEIVERS "receivers.txt"

struct upload_status 
{
    size_t bytes_read;
};

static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
    struct upload_status *upload_ctx = (struct upload_status *)userp;
    const char *data;
    size_t room = size * nmemb;

    if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1))
    {
        return 0;
    }

    //Using a plaintext payload so that this program can be used for different email lists without being rewritten.
    std::ifstream dataFile;
    std::ostringstream sstream;
    dataFile.open(PAYLOAD, std::fstream::in);
    sstream << dataFile.rdbuf();
    const std::string file_output(sstream.str());
    const char* email_body = file_output.c_str();

    const char* email = email_body;
    data = &email[upload_ctx->bytes_read];

    if (data)
    {
        size_t len = strlen(data);
        if(room < len)
            len = room;
        memcpy(ptr, data, len);
        upload_ctx->bytes_read += len;

        return len;
    }
    return 0;
}

    int main(void)
    {
        CURL *curl;
        CURLcode res = CURLE_OK;
        struct curl_slist *recipients = NULL;
        struct upload_status upload_ctx = { 0 };
        curl = curl_easy_init();
        if(curl)
        {
            //login
            curl_easy_setopt(curl, CURLOPT_USERNAME, FROM_ADDR);
            curl_easy_setopt(curl, CURLOPT_PASSWORD, FROM_PASS);

            //This is the URL for your mailserver
            
            curl_easy_setopt(curl, CURLOPT_URL, SMTP_SERVER);
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

            /* Note that this option isn't strictly required, omitting it will result
            * in libcurl sending the MAIL FROM command with empty sender data. All
            * autoresponses should have an empty reverse-path, and should be directed
             * to the address in the reverse-path which triggered them. Otherwise,
            * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
            * details.
            */
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM_ADDR);

            //Add receipients, 
            std::ifstream dataFile;
            //Once again using plaintext email list so that the emailer can easily be used with external output
            dataFile.open(RECEIVERS, std::fstream::in);
            if(dataFile.is_open())
            {
                std::string s_line;
                while(getline(dataFile, s_line))
                {
                    const char* c_line = s_line.c_str();
                    recipients = curl_slist_append(recipients, c_line);
                }
            }
            dataFile.close();

            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

            //Callback function to specify the payload
            //You could just use CURLOPT_READDATA option to specify a FILE pointer to read from.
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
            curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

            //Send the message
            res = curl_easy_perform(curl);

            //Check for errors
            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

            //Free the list of recipients
            curl_slist_free_all(recipients);

            //curl won't send the quit command until you call cleanup
            curl_easy_cleanup(curl);
        }

        return (int)res;
    }