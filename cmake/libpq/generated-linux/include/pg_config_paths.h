/* pg_config_paths.h — stub для frontend libpq build (без PG install layout).
 *
 * Реально из этих defines libpq frontend использует только PKGLIBDIR
 * (для dlopen libpq-oauth-N.so). У нас OAuth не используется — путь не критичен.
 *
 * Bootstrap-libpq-linux.sh может перезатереть этот файл при пересборке,
 * но фактически PG ./configure создаёт его в .vcxproj стиле для install,
 * который у нас отсутствует — поэтому коммитим stub.
 */

#define PGBINDIR "/usr/local/pgsql/bin"
#define PGSHAREDIR "/usr/local/pgsql/share"
#define SYSCONFDIR "/usr/local/pgsql/etc"
#define INCLUDEDIR "/usr/local/pgsql/include"
#define PKGINCLUDEDIR "/usr/local/pgsql/include"
#define INCLUDEDIRSERVER "/usr/local/pgsql/include/server"
#define LIBDIR "/usr/local/pgsql/lib"
#define PKGLIBDIR "/usr/local/pgsql/lib"
#define LOCALEDIR "/usr/local/pgsql/share/locale"
#define DOCDIR "/usr/local/pgsql/share/doc/"
#define HTMLDIR "/usr/local/pgsql/share/doc/"
#define MANDIR "/usr/local/pgsql/share/man"
