const char *guildToken = "sunucuları izlettirmek istediginiz hesabin tokeni";
const char *vanityToken = "sunucuya urlyi alıcak yetki verilmiş hesabın tokeni";
const char *serverIds[] = {"sunucu idleri"};
const char *webhookUrl = "webhook urlsi";
const char *vanities[] = {"istediginiz url"};
int heartbeatInterval = 0;
struct lws *socket = NULL;

void sendWebhook(const char *message) {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        json_t *json = json_object();
        json_object_set_new(json, "content", json_string(message));
        char *json_data = json_dumps(json, 0);

        curl_easy_setopt(curl, CURLOPT_URL, webhookUrl);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() başarısız: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        json_decref(json);
        free(json_data);
    }
    curl_global_cleanup();
}

void assignVanity(const char *serverId, const char *vanity) {
    CURL *curl;
    CURLcode res;
    struct timeval start, end;
    long elapsed;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Authorization", vanityToken);

        json_t *json = json_object();
        json_object_set_new(json, "code", json_string(vanity));
        char *json_data = json_dumps(json, 0);

        char url[256];
        snprintf(url, sizeof(url), "https://canary.discord.com/api/v7/guilds/%s/vanity-url", serverId);

        gettimeofday(&start, NULL);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);

        res = curl_easy_perform(curl);

        gettimeofday(&end, NULL);
        elapsed = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000;

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("discord.gg/%s MS: %ld\n", vanity, elapsed);
            char message[256];
            snprintf(message, sizeof(message), "discord.gg/%s MS: %ld", vanity, elapsed);
            sendWebhook(message);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        json_decref(json);
        free(json_data);
    }
    curl_global_cleanup();
}

int main() {
    printf("Merhaba snipper!");
    printf(guildToken);
    return 0;
}
