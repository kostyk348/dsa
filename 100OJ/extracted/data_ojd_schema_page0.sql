CREATE TABLE "sqlb_temp_table_1" (
	"id"	INTEGER NOT NULL UNIQUE,
	"text_handle"	TEXT,
	"chapter_frequency"	INTEGER DEFAULT 1,
	"in_rotation"	INTEGER NOT NULL DEFAULT 1,
	"can_register"	INTEGER NOT NULL DEFAULT 1,
	"editor_valid"	INTEGER NOT NULL DEFAULT 1,
	"allow_dupes"	INTEGER NOT NULL DEFAULT 0,
	"info_hidden"	INTEGER NOT NULL DEFAULT 0,
	"in_debug"	INTEGER NOT NULL DEFAULT 0,
	"json_params"	BLOB,
	PRIMARY KEY("id" AUTOINCREMENT)
)y%%otablefield_eventsfield_events)CREATE TABLE "field_events" (
	"id"	INTEGER NOT NULL UNIQUE,
	"text_handle"	TEXT,
	"chapter_frequency"	INTEGER DEFAULT 1,
	"in_rotation"	INTEGER NOT NULL DEFAULT 1,
	"can_register"	INTEGER NOT NULL DEFAULT 1,
	"editor_valid"	INTEGER NOT NULL DEFAULT 1,
	"allow_dupes"	INTEGER NOT NULL DEFAULT 0,
	"info_hidden"	INTEGER NOT NULL DEFAULT 0,
	"json_params"	BLOB, "in_daN//qWN%%qtablefield_eventsfield_events-CREATE TABLE "field_events" (
	"id"	INTEGER NOT NULL UNIQUE,
	"text_handle"	TEXT,
	"chapter_frequency"	INTEGER DEFAULT 1,
	"in_rotation"	INTEGER NOT NULL DEFAULT 1,
	"can_register"	INTEGER NOT NULL DEFAULT 1,
	"editor_valid"	INTEGER NOT NULL DEFAULT 1,
	"allow_dupes"	INTEGER NOT NULL DEFAULT 0,
	"info_hidden"	INTEGER NOT NULL DEFAULT 0,
	"in_debug"	INTEGER NOT NULL DEFAULT 0,
	"json_params"	BLOB,
	PRIMARY KEY("id" AUTOINCREMENT)
)

CREATE TABLE "shop_items" (
	"id"	INTEGER,
	"item_id"	TEXT UNIQUE,
	"old_enum_id"	INTEGER UNIQUE,
	"item_type"	INTEGER,
	"price"	INTEGER DEFAULT 0,
	"currency_type"	INTEGER DEFAULT 0,
	"steam_level_req"	INTEGER DEFAULT 0,
	"dlc_type"	INTEGER DEFAULT 0,
	"slot_width"	INTEGER DEFAULT 1,
	"exclude"	INTEGER DEFAULT 0,
	"auto_add"	INTEGER DEFAULT 1,
	"json_params"	BLOB,
	"sort_key"	TEXT,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "unique_item_sort_key" UNIQUE("sort_key")
)G<[5

CREATE TABLE shop_item_categories (
 item_id TEXT,
 category_id INTEGER,
 FOREIGN KEY (item_id)
	REFERENCES shop_items (item_id)
	ON UPDATE CASCADE
	ON DELETE CASCADE,
 FOREIGN KEY (category_id)
	REFERENCES shop_categories (id)
	ON UPDATE CASCADE
	ON DELETE CASCADE
 CONSTRAINT unique_item_category UNIQUE (item_id, category_id)
)P2++Ytablesqlite_sequencesqlite_sequenceCREATE TABLE sqlite_sequence(name,seq)1++utableshop_categoriesshop_categoriesCREATE TABLE shop_categories (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	label_define TEXT,
	tooltip_define TEXT
)),=

CREATE TABLE units (
    id        INTEGER PRIMARY KEY,
    id_game   INTEGER UNIQUE,
    id_string TEXT
)$&+tablebgmsbgmsCREATE TABLE bgms (id INTEGER PRIMARY KEY, string_id TEXT, pack_name TEXT, pack_order INTEGER, loop_start INTEGER, volume_mod REAL, event TEXT)R"--Wtableface_coordinatesface_coordinatesCREATE TABLE face_coordinates (id INTEGER PRIMARY KEY, unit_id INTEGER REFERENCES units (id_game), pos_x INTEGER, pos_y INTEGER, costume_id INTEGER, pose_id INTEGER)xKtabletitlestitlesCREATE TABLE titles (id INTEGER PRIMARY KEY, lang INTEGER, type INTEGER, string TEXT, sub TEXT))=

CREATE TABLE fonts (id INTEGER UNIQUE, handle TEXT UNIQUE, filename TEXT, PRIMARY KEY (id))