#include "db-manager.h"
#include "schema.h"
#include "project-object.h"
#include "task-object.h"
#include <stdio.h>
#include <time.h>
#include <glib-object.h>

G_DEFINE_TYPE(GTimerDBManager, gtimer_db_manager, G_TYPE_OBJECT)

GQuark gtimer_db_error_quark(void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string("gtimer-db-error-quark");
	return quark;
}

static char *get_today_date_string(void)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char buf[11];
	strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
	return g_strdup(buf);
}

static void gtimer_db_manager_finalize(GObject *object)
{
	GTimerDBManager *self = GTIMER_DB_MANAGER(object);
	if (self->db) {
		sqlite3_close(self->db);
		self->db = NULL;
	}
	G_OBJECT_CLASS(gtimer_db_manager_parent_class)->finalize(object);
}

static void gtimer_db_manager_class_init(GTimerDBManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gtimer_db_manager_finalize;
}

static void gtimer_db_manager_init(GTimerDBManager *self)
{
	(void)self;
}

sqlite3 *gtimer_db_manager_get_db(GTimerDBManager *self)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), NULL);
	return self->db;
}

GTimerDBManager *gtimer_db_manager_new(const char *path, GError **error)
{
	GTimerDBManager *self;
	int rc;
	char *errmsg = NULL;

	self = g_object_new(GTIMER_TYPE_DB_MANAGER, NULL);

	rc = sqlite3_open(path, &self->db);
	if (rc != SQLITE_OK) {
		g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
			    "Failed to open database: %s",
			    sqlite3_errmsg(self->db));
		sqlite3_close(self->db);
		self->db = NULL;
		g_object_unref(self);
		return NULL;
	}

	rc = sqlite3_exec(self->db, "PRAGMA foreign_keys = ON;", NULL, NULL,
			 &errmsg);
	if (rc != SQLITE_OK) {
		g_warning("Failed to enable foreign keys: %s", errmsg);
		sqlite3_free(errmsg);
	}

	rc = sqlite3_exec(self->db, schema_sql, NULL, NULL, &errmsg);
	if (rc != SQLITE_OK) {
		g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
			    "Failed to initialize schema: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(self->db);
		self->db = NULL;
		g_object_unref(self);
		return NULL;
	}
	rc = sqlite3_exec(self->db,
			  "ALTER TABLE tasks ADD COLUMN is_hidden INTEGER DEFAULT 0;",
			  NULL, NULL, &errmsg);
	if (rc == SQLITE_OK) {
		sqlite3_free(errmsg);
	} else if (rc != SQLITE_ERROR) {
		sqlite3_free(errmsg);
	}

	rc = sqlite3_exec(self->db,
			  "ALTER TABLE tasks ADD COLUMN is_timing INTEGER DEFAULT 0;",
			  NULL, NULL, &errmsg);
	if (rc == SQLITE_OK) {
		sqlite3_free(errmsg);
	} else if (rc != SQLITE_ERROR) {
		sqlite3_free(errmsg);
	}

	rc = sqlite3_exec(self->db,
			  "ALTER TABLE tasks ADD COLUMN last_start_time INTEGER;",
			  NULL, NULL, &errmsg);
	if (rc == SQLITE_OK) {
		sqlite3_free(errmsg);
	} else if (rc != SQLITE_ERROR) {
		sqlite3_free(errmsg);
	}

	return self;
}

static void gtimer_db_manager_create_task_internal(GTimerDBManager *self,
						   const char *name,
						   int project_id,
						   GError **error)
{
	sqlite3_stmt *stmt;
	int rc;

	rc = sqlite3_prepare_v2(self->db,
			       "INSERT INTO tasks (name, project_id) VALUES (?, ?);",
			       -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to prepare statement: %s",
			    sqlite3_errmsg(self->db));
		return;
	}

	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
	if (project_id != -1)
		sqlite3_bind_int(stmt, 2, project_id);
	else
		sqlite3_bind_null(stmt, 2);

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to insert task: %s",
			    sqlite3_errmsg(self->db));
	}
	sqlite3_finalize(stmt);
}

void gtimer_db_manager_create_task(GTimerDBManager *self, const char *name,
				   int project_id, GError **error)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	GError *local_error = NULL;
	gtimer_db_manager_create_task_internal(self, name, project_id,
					       &local_error);
	if (local_error) {
		g_propagate_error(error, local_error);
	}
}

void gtimer_db_manager_update_task(GTimerDBManager *self, int task_id,
				   const char *name, int project_id,
				   GError **error)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	const char *sql = "UPDATE tasks SET name = ?, project_id = ? WHERE id = ?;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to prepare statement: %s",
			    sqlite3_errmsg(self->db));
		return;
	}

	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
	if (project_id != -1)
		sqlite3_bind_int(stmt, 2, project_id);
	else
		sqlite3_bind_null(stmt, 2);
	sqlite3_bind_int(stmt, 3, task_id);

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to update task: %s",
			    sqlite3_errmsg(self->db));
	}
	sqlite3_finalize(stmt);
}

void gtimer_db_manager_delete_task(GTimerDBManager *self, int task_id,
				   GError **error)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	const char *sql = "DELETE FROM tasks WHERE id = ?;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to prepare statement: %s",
			    sqlite3_errmsg(self->db));
		return;
	}

	sqlite3_bind_int(stmt, 1, task_id);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to delete task: %s",
			    sqlite3_errmsg(self->db));
	}
	sqlite3_finalize(stmt);
}

void gtimer_db_manager_hide_task(GTimerDBManager *self, int task_id,
				 gboolean hidden, GError **error)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	const char *sql = "UPDATE tasks SET is_hidden = ? WHERE id = ?;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to prepare statement: %s",
			    sqlite3_errmsg(self->db));
		return;
	}

	sqlite3_bind_int(stmt, 1, hidden ? 1 : 0);
	sqlite3_bind_int(stmt, 2, task_id);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to hide task: %s",
			    sqlite3_errmsg(self->db));
	}
	sqlite3_finalize(stmt);
}

void gtimer_db_manager_create_project(GTimerDBManager *self,
				       const char *name, GError **error)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	const char *sql = "INSERT INTO projects (name) VALUES (?);";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to prepare statement: %s",
			    sqlite3_errmsg(self->db));
		return;
	}

	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to create project: %s",
			    sqlite3_errmsg(self->db));
	}
	sqlite3_finalize(stmt);
}

void gtimer_db_manager_update_project(GTimerDBManager *self, int project_id,
				       const char *name, GError **error)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	const char *sql = "UPDATE projects SET name = ? WHERE id = ?;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to prepare statement: %s",
			    sqlite3_errmsg(self->db));
		return;
	}

	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 2, project_id);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		g_set_error(error, GTIMER_DB_ERROR, GTIMER_DB_ERROR_SQL,
			    "Failed to update project: %s",
			    sqlite3_errmsg(self->db));
	}
	sqlite3_finalize(stmt);
}

GList *gtimer_db_manager_get_projects(GTimerDBManager *self)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), NULL);

	sqlite3_stmt *stmt;
	GList *projects = NULL;
	const char *sql = "SELECT id, name FROM projects ORDER BY name ASC;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		return NULL;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		const char *name = (const char *)sqlite3_column_text(stmt, 1);
		projects = g_list_append(projects,
					 gtimer_project_new(id, name));
	}

	sqlite3_finalize(stmt);
	return projects;
}

GList *gtimer_db_manager_get_hidden_tasks(GTimerDBManager *self)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), NULL);

	sqlite3_stmt *stmt;
	GList *tasks = NULL;
	const char *sql =
		"SELECT t.id, t.name, t.project_id, p.name, t.created_at "
		"FROM tasks t "
		"LEFT JOIN projects p ON t.project_id = p.id "
		"WHERE t.is_hidden = 1 ORDER BY t.name ASC;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		return NULL;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		const char *name = (const char *)sqlite3_column_text(stmt, 1);
		int project_id = sqlite3_column_int(stmt, 2);
		const char *project_name =
			(const char *)sqlite3_column_text(stmt, 3);
		gint64 created_at = sqlite3_column_int64(stmt, 4);
		tasks = g_list_append(
			tasks, gtimer_task_new(id, name, project_id,
						project_name, 0, 0, FALSE, TRUE, 0,
						created_at));
	}

	sqlite3_finalize(stmt);
	return tasks;
}

GList *gtimer_db_manager_get_all_tasks(GTimerDBManager *self)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), NULL);

	sqlite3_stmt *stmt;
	GList *tasks = NULL;
	const char *sql =
		"SELECT t.id, t.name, t.project_id, p.name, t.is_timing, t.is_hidden, "
		"  t.last_start_time, t.created_at "
		"FROM tasks t "
		"LEFT JOIN projects p ON t.project_id = p.id "
		"ORDER BY t.name ASC;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		return NULL;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		const char *name = (const char *)sqlite3_column_text(stmt, 1);
		int project_id = sqlite3_column_int(stmt, 2);
		const char *project_name =
			(const char *)sqlite3_column_text(stmt, 3);
		gboolean is_timing = sqlite3_column_int(stmt, 4) != 0;
		gboolean is_hidden = sqlite3_column_int(stmt, 5) != 0;
		gint64 last_start = sqlite3_column_int64(stmt, 6);
		gint64 created_at = sqlite3_column_int64(stmt, 7);
		GTimerTask *newtask = gtimer_task_new(id, name, project_id,
						project_name, 0, 0, is_timing,
						is_hidden, last_start, created_at);
		tasks = g_list_append(tasks, newtask);
	}

	sqlite3_finalize(stmt);
	return tasks;
}

void gtimer_report_row_free(GTimerReportRow *row)
{
	if (row) {
		g_free(row->task_name);
		g_free(row);
	}
}

GList *gtimer_db_manager_get_daily_report(GTimerDBManager *self, int year,
					   int month, int day)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), NULL);

	sqlite3_stmt *stmt;
	GList *report = NULL;
	char date_str[11];
	snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", year, month,
		 day);

	const char *sql =
		"SELECT t.name, d.seconds "
		"FROM tasks t "
		"JOIN daily_time d ON t.id = d.task_id "
		"WHERE d.date = ? ORDER BY d.seconds DESC;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		return NULL;

	sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_STATIC);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		GTimerReportRow *row = g_new0(GTimerReportRow, 1);
		row->task_name =
			g_strdup((const char *)sqlite3_column_text(stmt, 0));
		row->total_duration = sqlite3_column_int64(stmt, 1);
		report = g_list_append(report, row);
	}

	sqlite3_finalize(stmt);
	return report;
}

gint64 gtimer_db_manager_get_task_total_time(GTimerDBManager *self,
					      int task_id)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), 0);

	sqlite3_stmt *stmt;
	gint64 total_time = 0;
	const char *sql = "SELECT SUM(seconds) FROM daily_time WHERE task_id = ?;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		return 0;

	sqlite3_bind_int(stmt, 1, task_id);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		total_time = sqlite3_column_int64(stmt, 0);
	}

	sqlite3_finalize(stmt);
	return total_time;
}

gint64 gtimer_db_manager_get_task_today_time(GTimerDBManager *self,
					      int task_id)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), 0);

	sqlite3_stmt *stmt;
	gint64 today_time = 0;
	char *date_str = get_today_date_string();
	const char *sql =
		"SELECT seconds FROM daily_time WHERE task_id = ? AND date = ?;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, task_id);
		sqlite3_bind_text(stmt, 2, date_str, -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) == SQLITE_ROW) {
			today_time = sqlite3_column_int64(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}
	g_free(date_str);
	return today_time;
}

void gtimer_db_manager_add_task_time_for_date(GTimerDBManager *self,
					       int task_id,
					       const char *date_str,
					       gint64 seconds)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO daily_time (task_id, date, seconds) VALUES (?, ?, ?) "
		"ON CONFLICT(task_id, date) DO UPDATE SET seconds = seconds + excluded.seconds;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, task_id);
		sqlite3_bind_text(stmt, 2, date_str, -1, SQLITE_TRANSIENT);
		sqlite3_bind_int64(stmt, 3, seconds);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

void gtimer_db_manager_add_task_time(GTimerDBManager *self, int task_id,
				      gint64 seconds)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	char *date_str = get_today_date_string();
	gtimer_db_manager_add_task_time_for_date(self, task_id, date_str,
						 seconds);
	g_free(date_str);
}

void gtimer_db_manager_set_task_today_time(GTimerDBManager *self,
					   int task_id, gint64 seconds)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	char *date_str = get_today_date_string();
	const char *sql =
		"INSERT INTO daily_time (task_id, date, seconds) VALUES (?, ?, ?) "
		"ON CONFLICT(task_id, date) DO UPDATE SET seconds = excluded.seconds;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, task_id);
		sqlite3_bind_text(stmt, 2, date_str, -1, SQLITE_TRANSIENT);
		sqlite3_bind_int64(stmt, 3, seconds);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	g_free(date_str);
}

void gtimer_db_manager_start_task_timing(GTimerDBManager *self, int task_id)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	const char *sql =
		"UPDATE tasks SET is_timing = 1, last_start_time = strftime('%s', 'now') WHERE id = ?;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, task_id);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

void gtimer_db_manager_stop_task_timing(GTimerDBManager *self, int task_id)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	gint64 last_start = 0;

	const char *query_sql =
		"SELECT last_start_time FROM tasks WHERE id = ? AND is_timing = 1;";
	if (sqlite3_prepare_v2(self->db, query_sql, -1, &stmt, NULL) ==
	    SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, task_id);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			last_start = sqlite3_column_int64(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}

	if (last_start > 0) {
		gint64 now = time(NULL);

		struct tm start_tm = *localtime((time_t *)&last_start);
		struct tm now_tm = *localtime((time_t *)&now);

		if (start_tm.tm_mday == now_tm.tm_mday &&
		    start_tm.tm_mon == now_tm.tm_mon &&
		    start_tm.tm_year == now_tm.tm_year) {
			gtimer_db_manager_add_task_time(self, task_id,
						       now - last_start);
		} else {
			gint64 current = last_start;
			while (TRUE) {
				struct tm cur_tm =
					*localtime((time_t *)&current);
				char date_str[11];
				strftime(date_str, sizeof(date_str),
					 "%Y-%m-%d", &cur_tm);

				cur_tm.tm_hour = 23;
				cur_tm.tm_min = 59;
				cur_tm.tm_sec = 59;
				gint64 end_of_day = mktime(&cur_tm);

				if (end_of_day >= now) {
					gtimer_db_manager_add_task_time_for_date(
						self, task_id, date_str,
						now - current);
					break;
				} else {
					gtimer_db_manager_add_task_time_for_date(
						self, task_id, date_str,
						end_of_day - current + 1);
					current = end_of_day + 1;
				}
			}
		}
	}

	const char *update_sql =
		"UPDATE tasks SET is_timing = 0, last_start_time = NULL WHERE id = ?;";
	if (sqlite3_prepare_v2(self->db, update_sql, -1, &stmt, NULL) ==
	    SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, task_id);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

gboolean gtimer_db_manager_is_task_timing(GTimerDBManager *self, int task_id)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), FALSE);

	sqlite3_stmt *stmt;
	gboolean is_timing = FALSE;
	const char *sql = "SELECT is_timing FROM tasks WHERE id = ?;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		return FALSE;

	sqlite3_bind_int(stmt, 1, task_id);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		is_timing = sqlite3_column_int(stmt, 0) != 0;
	}

	sqlite3_finalize(stmt);
	return is_timing;
}

GList *gtimer_db_manager_get_annotations(GTimerDBManager *self, int task_id)
{
	g_return_val_if_fail(GTIMER_IS_DB_MANAGER(self), NULL);

	sqlite3_stmt *stmt;
	GList *annotations = NULL;
	const char *sql =
		"SELECT id, created_at, text FROM annotations WHERE task_id = ? ORDER BY created_at DESC;";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		return NULL;

	sqlite3_bind_int(stmt, 1, task_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		GTimerAnnotation *ann = g_new0(GTimerAnnotation, 1);
		ann->id = sqlite3_column_int64(stmt, 0);
		ann->task_id = task_id;
		ann->created_at = sqlite3_column_int64(stmt, 1);
		ann->text = g_strdup((const char *)sqlite3_column_text(stmt, 2));
		annotations = g_list_append(annotations, ann);
	}

	sqlite3_finalize(stmt);
	return annotations;
}

void gtimer_db_manager_add_annotation(GTimerDBManager *self, int task_id,
				       const char *text)
{
	g_return_if_fail(GTIMER_IS_DB_MANAGER(self));

	sqlite3_stmt *stmt;
	const char *sql =
		"INSERT INTO annotations (task_id, created_at, text) VALUES (?, strftime('%s', 'now'), ?);";
	int rc;

	rc = sqlite3_prepare_v2(self->db, sql, -1, &stmt, NULL);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, task_id);
		sqlite3_bind_text(stmt, 2, text, -1, SQLITE_TRANSIENT);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

void gtimer_annotation_free(GTimerAnnotation *annotation)
{
	if (annotation) {
		g_free(annotation->text);
		g_free(annotation);
	}
}
