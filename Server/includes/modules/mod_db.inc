// =============================================================================
//  mod_db.inc — MySQL database connection module
//
//  Owns the MySQL connection handle. Other modules use DB_GetHandle() to
//  issue their own queries. This is the only module that touches the
//  connection lifecycle.
//
//  Emits:
//    EVT_DB_CONNECTED — after successful connection
//
//  Public API:
//    DB_GetHandle()    — returns MySQL:handle
//    DB_IsConnected()  — returns bool
// =============================================================================

#if defined _mod_db_included
    #endinput
#endif
#define _mod_db_included

// -----------------------------------------------------------------------------
//  Connection config (hardcoded for dev — TODO: read from file later)
// -----------------------------------------------------------------------------

#define DB_HOST         "127.0.0.1"
#define DB_USER         "samp"
#define DB_PASS         "samppass"
#define DB_NAME         "samp_rpg"
#define DB_PORT         3306

// -----------------------------------------------------------------------------
//  State
// -----------------------------------------------------------------------------

static MySQL:g_DbHandle = MySQL:0;
static bool:g_DbConnected = false;

// -----------------------------------------------------------------------------
//  Public API
// -----------------------------------------------------------------------------

stock MySQL:DB_GetHandle()
{
    return g_DbHandle;
}

stock bool:DB_IsConnected()
{
    return g_DbConnected;
}

// -----------------------------------------------------------------------------
//  Init / Destroy
// -----------------------------------------------------------------------------

stock DB_Init()
{
    EventBus_RegisterModule("Database");

    // Configure connection options
    new MySQLOpt:options = mysql_init_options();
    mysql_set_option(options, SERVER_PORT, DB_PORT);
    mysql_set_option(options, AUTO_RECONNECT, true);
    mysql_set_option(options, POOL_SIZE, 2);
    mysql_set_option(options, SSL_ENABLE, false);

    // Connect (synchronous — blocks briefly on startup, which is fine)
    g_DbHandle = mysql_connect(DB_HOST, DB_USER, DB_PASS, DB_NAME, options);

    if (g_DbHandle == MySQL:0 || mysql_errno(g_DbHandle) != 0)
    {
        g_DbConnected = false;
        printf("[DB] ERROR: Failed to connect to MySQL %s@%s/%s (errno: %d)",
            DB_USER, DB_HOST, DB_NAME, mysql_errno(g_DbHandle));
        printf("[DB] Make sure MySQL is running: docker compose up -d mysql");
        return;
    }

    g_DbConnected = true;
    mysql_log(ALL);
    printf("[DB] Connected to MySQL %s@%s/%s (handle: %d)",
        DB_USER, DB_HOST, DB_NAME, _:g_DbHandle);

    // Notify other modules
    EventBus_Emit(EVT_DB_CONNECTED);
}

stock DB_Destroy()
{
    if (g_DbConnected)
    {
        mysql_close(g_DbHandle);
        g_DbConnected = false;
        printf("[DB] MySQL connection closed.");
    }
}
