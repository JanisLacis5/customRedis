#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "buffer_funcs.h"
#include "data_structures/dstr.h"

const size_t MAX_MESSAGE_LEN = 4096;

enum {
    TAG_INT = 0,
    TAG_STR = 1,
    TAG_ARR = 2,
    TAG_NULL = 3,
    TAG_ERROR = 4,
    TAG_DOUBLE = 5
};

static int read_all(int fd, uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t rec_len = read(fd, buf, n);
        if (rec_len <= 0) {
            return -1;
        }
        n -= rec_len;
        buf += rec_len;
    }
    return 0;
}

static int write_all(int fd, uint8_t *buf, size_t n) {
    while (n > 0) {
        ssize_t sent_len = write(fd, buf, n);
        if (sent_len <= 0) {
            return -1;
        }
        n -= sent_len;
        buf += sent_len;
    }
    return 0;
}

static void print_res(std::vector<uint8_t> &buf, size_t curr, const uint32_t total_len) {
    if (curr >= 4 + total_len) {
        return;
    }

    // Read the tag
    uint8_t tag = 0;
    memcpy(&tag, &buf[curr++], 1);

    if (tag == TAG_ARR) {
        // Read the size of the array
        uint32_t arr_size = 0;
        memcpy(&arr_size, &buf[curr], 4);
        curr += 4;

        printf("(arr) len=%d\n", arr_size);
    }
    else if (tag == TAG_STR || tag == TAG_ERROR) {
        // Read the size
        uint32_t str_size = 0;
        memcpy(&str_size, &buf[curr], 4);
        curr += 4;

        // Print the string
        dstr *str = dstr_init(str_size);
        memcpy(str->buf, &buf[curr], str_size);
        curr += str_size;

        printf("%s %s\n", (tag == TAG_STR ? "(str)" : "(err)"), str->buf);
    }
    else if (tag == TAG_DOUBLE) {
        // Read the double
        double data = 0;
        memcpy(&data, &buf[curr], 8);
        curr += 8;

        // Print it
        printf("(double) %f\n", data);
    }
    else if (tag == TAG_INT) {
        // Read the int
        uint32_t data = 0;
        memcpy(&data, &buf[curr], 4);
        curr += 4;

        printf("(int) %d\n", data);
    }
    else if (tag == TAG_NULL) {
        printf("(null)\n");
    }

    print_res(buf, curr, total_len);
}

static int handle_read(int fd) {
    std::vector<uint8_t> buf(4);

    // Read the total len
    int err = read_all(fd, &buf[0], 4);
    if (err) {
        printf("[client]: Error reading total_len\n");
        return -1;
    }
    uint32_t total_len = 0;
    memcpy(&total_len, &buf[0], 4);

    // Read everything
    buf.resize(4 + total_len);
    err = read_all(fd, &buf[4], total_len);
    if (err) {
        printf("[client]: Error reading the full message\n");
        return -1;
    }

    // Read response code
    // Skip the total_len + arr_tag(TAG_ARR) + arr_len(2) + response_type_tag(int)
    size_t curr = 4 + 1 + 4 + 1;
    uint32_t res_code = 0;
    memcpy(&res_code, &buf[curr], 4);
    curr += 4;

    print_res(buf, curr, total_len);
    return 0;
}

static int handle_write(int fd, std::vector<dstr*> &query) {
    std::vector<uint8_t> buf;

    // Calculate the total write buffer size
    uint32_t total_size = 5;  // initial tag + query.size()
    for (const dstr *token: query) {
        total_size += 5 + token->size; // tag(1) + size(4) + token_len(n)
    }

    if (total_size > MAX_MESSAGE_LEN) {
        printf("[client]: Message too large\n");
        return -1;
    }

    // Add the size header
    buf_append_u32(buf, total_size);

    // Add tag and len (always array because query can contain many arguments)
    buf_append_u8(buf, TAG_ARR);
    buf_append_u32(buf, (uint32_t)query.size());

    // Add each argument
    for (const dstr *token: query) {
        uint32_t len = token->size;
        // Add tag and len for the current token
        buf_append_u8(buf, TAG_STR);
        buf_append_u32(buf, len);

        // Add the value
        buf_append(buf, (uint8_t*)token->buf, len);
    }

    return write_all(fd, buf.data(), buf.size());
}

static int handle_loop(int fd) {
    std::vector<dstr*> cmd;
    char rawcmd[1024];

    if (fgets(rawcmd, 1024, stdin)) {
        // Remove the last '\n' char
        size_t len = strlen(rawcmd);
        if (len > 0) {
            rawcmd[len-1] = '\0';
        }

        char *tmp = strtok(rawcmd, " ");
        while (tmp) {
            size_t len = strlen(tmp);
            dstr *str = dstr_init(len);
            dstr_append(&str, tmp, len);
            cmd.push_back(str);
            tmp = strtok(NULL, " ");
        }
        int err = handle_write(fd, cmd);
        if (err) {
            close(fd);
            printf("[client]: Error sending the query\n");
            return -1;
        }
        err = handle_read(fd);
        if (err) {
            close(fd);
            printf("[client]: Error reading the response\n");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    // Create the socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return 1;
    }

    // connect to the socket
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(0);
    int conn = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (conn == -1) {
        printf("[client]: Connection failed\n");
        return 1;
    }

    while (1) {
        int err = handle_loop(fd);
        if (err) {
            return err;
        }
    }
}
