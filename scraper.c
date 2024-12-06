#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define MAX_LINKS 1000

typedef struct {
    char *data;
    size_t size;
} MemoryChunk;

// Write callback to handle data fetched by libcurl
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    MemoryChunk *mem = (MemoryChunk *)userp;

    char *ptr = realloc(mem->data, mem->size + total_size + 1);
    if (ptr == NULL) {
        printf("Not enough memory!\n");
        return 0;
    }
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, total_size);
    mem->size += total_size;
    mem->data[mem->size] = '\0';

    return total_size;
}

// Fetch page content
char *fetch_page(const char *url) {
    CURL *curl;
    CURLcode res;
    MemoryChunk chunk = {NULL, 0};

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Curl initialization failed!\n");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Curl error: %s\n", curl_easy_strerror(res));
        free(chunk.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    curl_easy_cleanup(curl);
    return chunk.data;
}

// Extract links (basic string-based parser)
void extract_links(const char *html, FILE *output_file, char *base_url) {
    const char *tag = "<a href=\"";
    const char *end_tag = "\"";
    char *start = strstr(html, tag);
    while (start) {
        start += strlen(tag);
        char *end = strstr(start, end_tag);
        if (end) {
            size_t link_length = end - start;
            char link[2048];
            snprintf(link, link_length + 1, "%s", start);

            if (strstr(link, "http") == link) { // Simple check for absolute URLs
                printf("Found link: %s\n", link);
                fprintf(output_file, "%s\n", link);
            } else if (link[0] == '/') { // Relative URL, prepend base URL
                printf("Found link: %s%s\n", base_url, link);
                fprintf(output_file, "%s%s\n", base_url, link);
            }
        }
        start = strstr(start, tag);
    }
}

// Recursive scraper
void scrape(const char *url, int depth, FILE *output_file, char visited_links[MAX_LINKS][2048], int *visited_count) {
    if (depth <= 0 || *visited_count >= MAX_LINKS) return;

    // Check if URL was already visited
    for (int i = 0; i < *visited_count; i++) {
        if (strcmp(url, visited_links[i]) == 0) return;
    }

    strcpy(visited_links[*visited_count], url);
    (*visited_count)++;

    char *html = fetch_page(url);
    if (!html) return;

    extract_links(html, output_file, url);

    // Recursive call for more links (dummy logic, for demonstration purposes)
    char *dummy_links[] = {"http://example.com", "http://example.com/page2"};
    for (int i = 0; i < 2; i++) {
        scrape(dummy_links[i], depth - 1, output_file, visited_links, visited_count);
    }

    free(html);
}

int main() {
    const char *start_url = "http://example.com";
    FILE *output_file = fopen("links.txt", "w");
    if (!output_file) {
        perror("Failed to open output file");
        return 1;
    }

    char visited_links[MAX_LINKS][2048] = {0};
    int visited_count = 0;

    scrape(start_url, 2, output_file, visited_links, &visited_count);

    fclose(output_file);
    printf("Scraping completed. Links saved to links.txt\n");

    return 0;
}
