/*
 * LDBC (Linked Data Benchmark Council) Social Network Benchmark
 * Implementation for SQLite Graph Extension
 * 
 * This implements a subset of the LDBC Social Network Benchmark
 * to validate performance characteristics of the graph extension.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <sqlite3.h>
#include "graph.h"

/* Benchmark configuration */
#define MAX_PERSONS 10000
#define MAX_POSTS 20000
#define MAX_COMMENTS 50000
#define MAX_FRIENDSHIPS 30000
#define MAX_LIKES 100000

/* Result structure for benchmark queries */
typedef struct LdbcResult {
    int query_id;
    double execution_time_ms;
    int result_count;
    char *error_message;
} LdbcResult;

/* Benchmark data structures */
typedef struct Person {
    sqlite3_int64 id;
    char *firstName;
    char *lastName;
    char *email;
    char *birthday;
    char *locationIP;
    char *browserUsed;
    sqlite3_int64 cityId;
    char *languages;
} Person;

typedef struct Post {
    sqlite3_int64 id;
    sqlite3_int64 creatorId;
    char *creationDate;
    char *content;
    char *language;
    char *locationIP;
    char *browserUsed;
} Post;

typedef struct Comment {
    sqlite3_int64 id;
    sqlite3_int64 creatorId;
    char *creationDate;
    char *content;
    sqlite3_int64 replyToPostId;
    sqlite3_int64 replyToCommentId;
    char *locationIP;
    char *browserUsed;
} Comment;

/* Utility functions */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static char *generate_random_string(int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *str = malloc(length + 1);
    if (!str) return NULL;
    
    for (int i = 0; i < length; i++) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    str[length] = '\0';
    return str;
}

static char *generate_date_string(void) {
    int year = 1980 + (rand() % 40);
    int month = 1 + (rand() % 12);
    int day = 1 + (rand() % 28);
    char *date = malloc(11);
    if (!date) return NULL;
    snprintf(date, 11, "%04d-%02d-%02d", year, month, day);
    return date;
}

static char *generate_datetime_string(void) {
    char *date = generate_date_string();
    if (!date) return NULL;
    
    char *datetime = malloc(20);
    if (!datetime) {
        free(date);
        return NULL;
    }
    
    int hour = rand() % 24;
    int minute = rand() % 60;
    int second = rand() % 60;
    
    snprintf(datetime, 20, "%sT%02d:%02d:%02d", date, hour, minute, second);
    free(date);
    return datetime;
}

/* Data generation functions */
static Person *generate_person(sqlite3_int64 id) {
    Person *person = calloc(1, sizeof(Person));
    if (!person) return NULL;
    
    person->id = id;
    person->firstName = generate_random_string(8);
    person->lastName = generate_random_string(10);
    
    char *email = malloc(50);
    if (email) {
        snprintf(email, 50, "%s.%s@example.com", 
                person->firstName, person->lastName);
        person->email = email;
    }
    
    person->birthday = generate_date_string();
    person->locationIP = malloc(16);
    if (person->locationIP) {
        snprintf(person->locationIP, 16, "192.168.%d.%d", 
                rand() % 256, rand() % 256);
    }
    
    const char *browsers[] = {"Chrome", "Firefox", "Safari", "Edge"};
    person->browserUsed = strdup(browsers[rand() % 4]);
    
    person->cityId = 1 + (rand() % 100);
    
    const char *languages[] = {"en", "es", "fr", "de", "it", "pt"};
    person->languages = strdup(languages[rand() % 6]);
    
    return person;
}

static Post *generate_post(sqlite3_int64 id, sqlite3_int64 creator_id) {
    Post *post = calloc(1, sizeof(Post));
    if (!post) return NULL;
    
    post->id = id;
    post->creatorId = creator_id;
    post->creationDate = generate_datetime_string();
    post->content = generate_random_string(100 + rand() % 400);
    
    const char *languages[] = {"en", "es", "fr", "de", "it", "pt"};
    post->language = strdup(languages[rand() % 6]);
    
    post->locationIP = malloc(16);
    if (post->locationIP) {
        snprintf(post->locationIP, 16, "10.0.%d.%d", 
                rand() % 256, rand() % 256);
    }
    
    const char *browsers[] = {"Chrome", "Firefox", "Safari", "Edge"};
    post->browserUsed = strdup(browsers[rand() % 4]);
    
    return post;
}

static Comment *generate_comment(sqlite3_int64 id, sqlite3_int64 creator_id) {
    Comment *comment = calloc(1, sizeof(Comment));
    if (!comment) return NULL;
    
    comment->id = id;
    comment->creatorId = creator_id;
    comment->creationDate = generate_datetime_string();
    comment->content = generate_random_string(50 + rand() % 200);
    
    /* 70% chance of replying to a post, 30% to another comment */
    if (rand() % 100 < 70) {
        comment->replyToPostId = 1 + (rand() % MAX_POSTS);
        comment->replyToCommentId = 0;
    } else {
        comment->replyToPostId = 0;
        comment->replyToCommentId = 1 + (rand() % (id - 1));
    }
    
    comment->locationIP = malloc(16);
    if (comment->locationIP) {
        snprintf(comment->locationIP, 16, "172.16.%d.%d", 
                rand() % 256, rand() % 256);
    }
    
    const char *browsers[] = {"Chrome", "Firefox", "Safari", "Edge"};
    comment->browserUsed = strdup(browsers[rand() % 4]);
    
    return comment;
}

/* Free functions */
static void free_person(Person *person) {
    if (!person) return;
    free(person->firstName);
    free(person->lastName);
    free(person->email);
    free(person->birthday);
    free(person->locationIP);
    free(person->browserUsed);
    free(person->languages);
    free(person);
}

static void free_post(Post *post) {
    if (!post) return;
    free(post->creationDate);
    free(post->content);
    free(post->language);
    free(post->locationIP);
    free(post->browserUsed);
    free(post);
}

static void free_comment(Comment *comment) {
    if (!comment) return;
    free(comment->creationDate);
    free(comment->content);
    free(comment->locationIP);
    free(comment->browserUsed);
    free(comment);
}

/* Database initialization */
static int create_ldbc_graph(sqlite3 *db) {
    char *zErrMsg = NULL;
    int rc;
    
    /* Create the graph virtual table */
    const char *sql = "CREATE VIRTUAL TABLE ldbc_graph USING graph("
                     "cache_size=10000, max_depth=10, thread_pool_size=4)";
    
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error creating graph: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return rc;
    }
    
    /* Create indexes for performance */
    const char *indexes[] = {
        "CREATE INDEX idx_ldbc_node_labels ON ldbc_graph(labels) WHERE edge_id IS NULL",
        "CREATE INDEX idx_ldbc_edge_types ON ldbc_graph(edge_type) WHERE edge_id IS NOT NULL",
        "CREATE INDEX idx_ldbc_from_to ON ldbc_graph(from_id, to_id) WHERE edge_id IS NOT NULL",
        "CREATE INDEX idx_ldbc_person_name ON ldbc_graph("
        "  json_extract(properties, '$.firstName'),"
        "  json_extract(properties, '$.lastName')"
        ") WHERE edge_id IS NULL AND json_extract(labels, '$[0]') = 'Person'",
        NULL
    };
    
    for (int i = 0; indexes[i]; i++) {
        rc = sqlite3_exec(db, indexes[i], NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Error creating index: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return rc;
        }
    }
    
    return SQLITE_OK;
}

/* Data loading functions */
static int load_persons(sqlite3 *db) {
    printf("Loading %d persons...\n", MAX_PERSONS);
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO ldbc_graph (node_id, labels, properties) VALUES (?, ?, ?)";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    
    for (int i = 1; i <= MAX_PERSONS; i++) {
        Person *person = generate_person(i);
        if (!person) continue;
        
        /* Create JSON properties */
        char *properties = malloc(1024);
        if (properties) {
            snprintf(properties, 1024,
                "{"
                "\"firstName\":\"%s\","
                "\"lastName\":\"%s\","
                "\"email\":\"%s\","
                "\"birthday\":\"%s\","
                "\"locationIP\":\"%s\","
                "\"browserUsed\":\"%s\","
                "\"cityId\":%lld,"
                "\"languages\":\"%s\""
                "}",
                person->firstName, person->lastName, person->email,
                person->birthday, person->locationIP, person->browserUsed,
                person->cityId, person->languages);
        }
        
        sqlite3_bind_int64(stmt, 1, person->id);
        sqlite3_bind_text(stmt, 2, "[\"Person\"]", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, properties, -1, SQLITE_TRANSIENT);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Error inserting person %d: %s\n", i, sqlite3_errmsg(db));
        }
        
        sqlite3_reset(stmt);
        free(properties);
        free_person(person);
        
        if (i % 1000 == 0) {
            printf("  Loaded %d persons\n", i);
        }
    }
    
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    
    printf("Finished loading persons.\n");
    return SQLITE_OK;
}

static int load_posts(sqlite3 *db) {
    printf("Loading %d posts...\n", MAX_POSTS);
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO ldbc_graph (node_id, labels, properties) VALUES (?, ?, ?)";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    
    for (int i = 1; i <= MAX_POSTS; i++) {
        sqlite3_int64 creator_id = 1 + (rand() % MAX_PERSONS);
        Post *post = generate_post(MAX_PERSONS + i, creator_id);
        if (!post) continue;
        
        char *properties = malloc(1024);
        if (properties) {
            snprintf(properties, 1024,
                "{"
                "\"creatorId\":%lld,"
                "\"creationDate\":\"%s\","
                "\"content\":\"%s\","
                "\"language\":\"%s\","
                "\"locationIP\":\"%s\","
                "\"browserUsed\":\"%s\""
                "}",
                post->creatorId, post->creationDate, post->content,
                post->language, post->locationIP, post->browserUsed);
        }
        
        sqlite3_bind_int64(stmt, 1, post->id);
        sqlite3_bind_text(stmt, 2, "[\"Post\"]", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, properties, -1, SQLITE_TRANSIENT);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Error inserting post %d: %s\n", i, sqlite3_errmsg(db));
        }
        
        sqlite3_reset(stmt);
        free(properties);
        free_post(post);
        
        if (i % 1000 == 0) {
            printf("  Loaded %d posts\n", i);
        }
    }
    
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    
    printf("Finished loading posts.\n");
    return SQLITE_OK;
}

static int load_friendships(sqlite3 *db) {
    printf("Loading %d friendships...\n", MAX_FRIENDSHIPS);
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO ldbc_graph (edge_id, from_id, to_id, edge_type, weight, properties) VALUES (?, ?, ?, ?, ?, ?)";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    
    for (int i = 1; i <= MAX_FRIENDSHIPS; i++) {
        sqlite3_int64 person1 = 1 + (rand() % MAX_PERSONS);
        sqlite3_int64 person2 = 1 + (rand() % MAX_PERSONS);
        
        if (person1 == person2) continue;
        
        char *creation_date = generate_datetime_string();
        char *properties = malloc(256);
        if (properties && creation_date) {
            snprintf(properties, 256, "{\"creationDate\":\"%s\"}", creation_date);
        }
        
        sqlite3_bind_int64(stmt, 1, MAX_PERSONS + MAX_POSTS + i);
        sqlite3_bind_int64(stmt, 2, person1);
        sqlite3_bind_int64(stmt, 3, person2);
        sqlite3_bind_text(stmt, 4, "KNOWS", -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 5, 1.0);
        sqlite3_bind_text(stmt, 6, properties ? properties : "{}", -1, SQLITE_TRANSIENT);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Error inserting friendship %d: %s\n", i, sqlite3_errmsg(db));
        }
        
        sqlite3_reset(stmt);
        free(properties);
        free(creation_date);
        
        if (i % 1000 == 0) {
            printf("  Loaded %d friendships\n", i);
        }
    }
    
    sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    
    printf("Finished loading friendships.\n");
    return SQLITE_OK;
}

/* LDBC Query implementations */

/* IS1: Find person by first name */
static LdbcResult *ldbc_is1_find_person_by_name(sqlite3 *db, const char *firstName) {
    LdbcResult *result = calloc(1, sizeof(LdbcResult));
    if (!result) return NULL;
    
    result->query_id = 1;
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT node_id, properties "
        "FROM ldbc_graph "
        "WHERE edge_id IS NULL "
        "  AND json_extract(labels, '$[0]') = 'Person' "
        "  AND json_extract(properties, '$.firstName') = ? "
        "ORDER BY json_extract(properties, '$.lastName'), "
        "         json_extract(properties, '$.birthday') "
        "LIMIT 20";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    sqlite3_bind_text(stmt, 1, firstName, -1, SQLITE_STATIC);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    
    result->result_count = count;
    result->execution_time_ms = get_time_ms() - start_time;
    
    sqlite3_finalize(stmt);
    return result;
}

/* IS2: Find recent posts by person */
static LdbcResult *ldbc_is2_recent_posts_by_person(sqlite3 *db, sqlite3_int64 personId) {
    LdbcResult *result = calloc(1, sizeof(LdbcResult));
    if (!result) return NULL;
    
    result->query_id = 2;
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT p.node_id, p.properties "
        "FROM ldbc_graph p "
        "WHERE p.edge_id IS NULL "
        "  AND json_extract(p.labels, '$[0]') = 'Post' "
        "  AND json_extract(p.properties, '$.creatorId') = ? "
        "ORDER BY json_extract(p.properties, '$.creationDate') DESC "
        "LIMIT 10";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    sqlite3_bind_int64(stmt, 1, personId);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    
    result->result_count = count;
    result->execution_time_ms = get_time_ms() - start_time;
    
    sqlite3_finalize(stmt);
    return result;
}

/* IS3: Find friends of person */
static LdbcResult *ldbc_is3_friends_of_person(sqlite3 *db, sqlite3_int64 personId) {
    LdbcResult *result = calloc(1, sizeof(LdbcResult));
    if (!result) return NULL;
    
    result->query_id = 3;
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT friend.node_id, friend.properties "
        "FROM ldbc_graph friendship "
        "JOIN ldbc_graph friend ON friend.node_id = "
        "  CASE WHEN friendship.from_id = ? THEN friendship.to_id "
        "       ELSE friendship.from_id END "
        "WHERE friendship.edge_type = 'KNOWS' "
        "  AND (friendship.from_id = ? OR friendship.to_id = ?) "
        "  AND friend.edge_id IS NULL "
        "ORDER BY json_extract(friend.properties, '$.firstName'), "
        "         json_extract(friend.properties, '$.lastName') "
        "LIMIT 50";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    sqlite3_bind_int64(stmt, 1, personId);
    sqlite3_bind_int64(stmt, 2, personId);
    sqlite3_bind_int64(stmt, 3, personId);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    
    result->result_count = count;
    result->execution_time_ms = get_time_ms() - start_time;
    
    sqlite3_finalize(stmt);
    return result;
}

/* IS4: Find content with tag */
static LdbcResult *ldbc_is4_content_with_tag(sqlite3 *db, const char *tagName) {
    LdbcResult *result = calloc(1, sizeof(LdbcResult));
    if (!result) return NULL;
    
    result->query_id = 4;
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT node_id, properties "
        "FROM ldbc_graph "
        "WHERE edge_id IS NULL "
        "  AND (json_extract(labels, '$[0]') = 'Post' OR "
        "       json_extract(labels, '$[0]') = 'Comment') "
        "  AND json_extract(properties, '$.content') LIKE ? "
        "ORDER BY json_extract(properties, '$.creationDate') DESC "
        "LIMIT 20";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    char *pattern = malloc(strlen(tagName) + 10);
    if (pattern) {
        snprintf(pattern, strlen(tagName) + 10, "%%%s%%", tagName);
        sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_TRANSIENT);
        free(pattern);
    }
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    
    result->result_count = count;
    result->execution_time_ms = get_time_ms() - start_time;
    
    sqlite3_finalize(stmt);
    return result;
}

/* IS5: Find forum members */
static LdbcResult *ldbc_is5_forum_members(sqlite3 *db, sqlite3_int64 forumId) {
    LdbcResult *result = calloc(1, sizeof(LdbcResult));
    if (!result) return NULL;
    
    result->query_id = 5;
    double start_time = get_time_ms();
    
    /* For this benchmark, we'll simulate forum membership through friend networks */
    sqlite3_stmt *stmt;
    const char *sql = 
        "WITH RECURSIVE friend_network AS ( "
        "  SELECT ? as person_id, 0 as depth "
        "  UNION ALL "
        "  SELECT "
        "    CASE WHEN f.from_id = fn.person_id THEN f.to_id "
        "         ELSE f.from_id END as person_id, "
        "    fn.depth + 1 as depth "
        "  FROM friend_network fn "
        "  JOIN ldbc_graph f ON (f.from_id = fn.person_id OR f.to_id = fn.person_id) "
        "  WHERE f.edge_type = 'KNOWS' AND fn.depth < 2 "
        ") "
        "SELECT DISTINCT p.node_id, p.properties "
        "FROM friend_network fn "
        "JOIN ldbc_graph p ON p.node_id = fn.person_id "
        "WHERE p.edge_id IS NULL "
        "LIMIT 20";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    sqlite3_bind_int64(stmt, 1, forumId);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    
    result->result_count = count;
    result->execution_time_ms = get_time_ms() - start_time;
    
    sqlite3_finalize(stmt);
    return result;
}

/* IS6: Find replies to post */
static LdbcResult *ldbc_is6_replies_to_post(sqlite3 *db, sqlite3_int64 postId) {
    LdbcResult *result = calloc(1, sizeof(LdbcResult));
    if (!result) return NULL;
    
    result->query_id = 6;
    double start_time = get_time_ms();
    
    /* For this benchmark, we use graph traversal to find comment threads */
    sqlite3_stmt *stmt;
    const char *sql = 
        "WITH RECURSIVE comment_thread AS ( "
        "  SELECT node_id, 0 as depth "
        "  FROM ldbc_graph "
        "  WHERE edge_id IS NULL "
        "    AND json_extract(labels, '$[0]') = 'Comment' "
        "    AND json_extract(properties, '$.replyToPostId') = ? "
        "  UNION ALL "
        "  SELECT c.node_id, ct.depth + 1 "
        "  FROM comment_thread ct "
        "  JOIN ldbc_graph c ON c.edge_id IS NULL "
        "    AND json_extract(c.labels, '$[0]') = 'Comment' "
        "    AND json_extract(c.properties, '$.replyToCommentId') = ct.node_id "
        "  WHERE ct.depth < 3 "
        ") "
        "SELECT ct.node_id, c.properties, ct.depth "
        "FROM comment_thread ct "
        "JOIN ldbc_graph c ON c.node_id = ct.node_id "
        "ORDER BY ct.depth, json_extract(c.properties, '$.creationDate') "
        "LIMIT 100";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    sqlite3_bind_int64(stmt, 1, postId);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    
    result->result_count = count;
    result->execution_time_ms = get_time_ms() - start_time;
    
    sqlite3_finalize(stmt);
    return result;
}

/* IS7: Find replies to comments */
static LdbcResult *ldbc_is7_replies_to_comment(sqlite3 *db, sqlite3_int64 commentId) {
    LdbcResult *result = calloc(1, sizeof(LdbcResult));
    if (!result) return NULL;
    
    result->query_id = 7;
    double start_time = get_time_ms();
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT node_id, properties "
        "FROM ldbc_graph "
        "WHERE edge_id IS NULL "
        "  AND json_extract(labels, '$[0]') = 'Comment' "
        "  AND json_extract(properties, '$.replyToCommentId') = ? "
        "ORDER BY json_extract(properties, '$.creationDate') "
        "LIMIT 50";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result->error_message = strdup(sqlite3_errmsg(db));
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    sqlite3_bind_int64(stmt, 1, commentId);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    
    result->result_count = count;
    result->execution_time_ms = get_time_ms() - start_time;
    
    sqlite3_finalize(stmt);
    return result;
}

/* Benchmark runner */
static void run_benchmark_suite(sqlite3 *db) {
    printf("\n=== LDBC Social Network Benchmark Suite ===\n\n");
    
    LdbcResult *results[7];
    
    /* Run Interactive Short queries */
    printf("Running Interactive Short (IS) queries...\n");
    
    results[0] = ldbc_is1_find_person_by_name(db, "John");
    printf("IS1 - Find person by name: %.2f ms (%d results)\n", 
           results[0]->execution_time_ms, results[0]->result_count);
    
    results[1] = ldbc_is2_recent_posts_by_person(db, 1);
    printf("IS2 - Recent posts by person: %.2f ms (%d results)\n", 
           results[1]->execution_time_ms, results[1]->result_count);
    
    results[2] = ldbc_is3_friends_of_person(db, 1);
    printf("IS3 - Friends of person: %.2f ms (%d results)\n", 
           results[2]->execution_time_ms, results[2]->result_count);
    
    results[3] = ldbc_is4_content_with_tag(db, "test");
    printf("IS4 - Content with tag: %.2f ms (%d results)\n", 
           results[3]->execution_time_ms, results[3]->result_count);
    
    results[4] = ldbc_is5_forum_members(db, 1);
    printf("IS5 - Forum members: %.2f ms (%d results)\n", 
           results[4]->execution_time_ms, results[4]->result_count);
    
    results[5] = ldbc_is6_replies_to_post(db, MAX_PERSONS + 1);
    printf("IS6 - Replies to post: %.2f ms (%d results)\n", 
           results[5]->execution_time_ms, results[5]->result_count);
    
    results[6] = ldbc_is7_replies_to_comment(db, MAX_PERSONS + MAX_POSTS + 1);
    printf("IS7 - Replies to comment: %.2f ms (%d results)\n", 
           results[6]->execution_time_ms, results[6]->result_count);
    
    /* Calculate statistics */
    double total_time = 0;
    int total_results = 0;
    int passed_queries = 0;
    
    for (int i = 0; i < 7; i++) {
        if (results[i]->error_message == NULL) {
            total_time += results[i]->execution_time_ms;
            total_results += results[i]->result_count;
            passed_queries++;
        } else {
            printf("ERROR in IS%d: %s\n", i + 1, results[i]->error_message);
        }
    }
    
    printf("\n=== Benchmark Summary ===\n");
    printf("Queries passed: %d/7\n", passed_queries);
    printf("Total execution time: %.2f ms\n", total_time);
    printf("Average query time: %.2f ms\n", total_time / passed_queries);
    printf("Total results returned: %d\n", total_results);
    
    /* Performance targets validation */
    printf("\n=== Performance Validation ===\n");
    double avg_time = total_time / passed_queries;
    if (avg_time < 100.0) {
        printf("✓ PASS: Average query time (%.2f ms) < 100 ms target\n", avg_time);
    } else {
        printf("✗ FAIL: Average query time (%.2f ms) >= 100 ms target\n", avg_time);
    }
    
    if (passed_queries == 7) {
        printf("✓ PASS: All queries executed successfully\n");
    } else {
        printf("✗ FAIL: %d queries failed\n", 7 - passed_queries);
    }
    
    /* Cleanup */
    for (int i = 0; i < 7; i++) {
        if (results[i]->error_message) {
            free(results[i]->error_message);
        }
        free(results[i]);
    }
}

/* Main benchmark function */
int main(int argc, char **argv) {
    sqlite3 *db;
    int rc;
    
    printf("LDBC Social Network Benchmark for SQLite Graph Extension\n");
    printf("========================================================\n");
    
    /* Initialize random seed */
    srand(time(NULL));
    
    /* Open database */
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    /* Load graph extension */
    char *zErrMsg = NULL;
    rc = sqlite3_load_extension(db, "./graph_extension", NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error loading extension: %s\n", 
                zErrMsg ? zErrMsg : sqlite3_errmsg(db));
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }
    
    /* Create graph schema */
    printf("Creating LDBC graph schema...\n");
    rc = create_ldbc_graph(db);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    
    /* Load test data */
    printf("Loading test data...\n");
    double start_time = get_time_ms();
    
    load_persons(db);
    load_posts(db);
    load_friendships(db);
    
    double load_time = get_time_ms() - start_time;
    printf("Data loading completed in %.2f seconds\n", load_time / 1000.0);
    
    /* Analyze data for optimization */
    printf("Analyzing data for query optimization...\n");
    sqlite3_exec(db, "ANALYZE ldbc_graph", NULL, NULL, NULL);
    
    /* Run benchmarks */
    run_benchmark_suite(db);
    
    /* Cleanup */
    sqlite3_close(db);
    
    printf("\nBenchmark completed successfully.\n");
    return 0;
}