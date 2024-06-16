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

static int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch(reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Connected to Discord Gateway\n");
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            json_t *json = json_loads((const char *)in, 0, NULL);
            json_t *op = json_object_get(json, "op");

            if(json_integer_value(op) == 0) {
                const char *t = json_string_value(json_object_get(json, "t"));
                if(strcmp(t, "GUILD_UPDATE") == 0 || strcmp(t, "GUILD_DELETE") == 0) {
                    const char *serverId = json_string_value(json_object_get(json, "d.id"));
                    for(size_t i = 0; i < sizeof(vanities) / sizeof(vanities[0]); ++i) {
                        assignVanity(serverId, vanities[i]);
                    }
                }
            } else if(json_integer_value(op) == 10) {
                json_t *d = json_object_get(json, "d");
                heartbeatInterval = json_integer_value(json_object_get(d, "heartbeat_interval"));
                lws_callback_on_writable(wsi);
            }
            json_decref(json);
            break;
        }

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            json_t *json = json_object();
            json_object_set_new(json, "op", json_integer(1));
            json_object_set_new(json, "d", json_null());
            char *json_data = json_dumps(json, 0);
            unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + strlen(json_data) + LWS_SEND_BUFFER_POST_PADDING];
            memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], json_data, strlen(json_data));
            lws_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], strlen(json_data), LWS_WRITE_TEXT);
            json_decref(json);
            free(json_data);
            break;
        }

        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("WebSocket closed. Reconnecting...\n");
            lws_cancel_service(lws_get_context(wsi));
            usleep(1000000);
            // reconnect logic here
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("WebSocket error. Exiting...\n");
            exit(EXIT_FAILURE);
            break;

        default:
            break;
    }

    return 0;
}

void connectToWebSocket() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);
    if(!context) {
        fprintf(stderr, "Creating libwebsocket context failed\n");
        return;
    }

    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = context;
    ccinfo.address = "gateway.discord.gg";
    ccinfo.port = 443;
    ccinfo.path = "/";
    ccinfo.ssl_connection = LCCSCF_USE_SSL;
    ccinfo.protocol = protocols[0].name;
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.ietf_version_or_minus_one = -1;

    socket = lws_client_connect_via_info(&ccinfo);
    if(!socket) {
        fprintf(stderr, "WebSocket connection failed\n");
        lws_context_destroy(context);
        return;
    }

    while(lws_service(context, 1000) >= 0);

    lws_context_destroy(context);
}

int main() {
    printf("Merhaba snipper!");
    printf(guildToken);
    return 0;
}
