
CREATE DATABASE "photobase";
\connect "photobase";

DROP TABLE IF EXISTS "photo_images";
DROP SEQUENCE IF EXISTS photo_images_id_seq;
CREATE SEQUENCE photo_images_id_seq INCREMENT 1 MINVALUE 1 MAXVALUE 2147483647 CACHE 1;

CREATE TABLE "photobase"."photo_images" (
    "id" integer DEFAULT nextval('photo_images_id_seq') NOT NULL,
    "file" character varying(255) DEFAULT '' NOT NULL,
    "date_available" timestamptz DEFAULT '1970-01-01 00:00:00+03' NOT NULL,
    "date_creation" timestamptz,
    "name" character varying(255),
    "comment" text,
    "device" character varying(255),
    "maker" character varying(255),
    "filesize" numeric,
    "width" integer,
    "height" integer,
    "ext" character varying(4),
    "path" character varying(255) DEFAULT '' NOT NULL,
    "media_name" character varying(255) DEFAULT '' NOT NULL,
    "md5sum" character(32),
    "rotation" smallint,
    "latitude" double precision,
    "longitude" double precision,
    "lastmodified" timestamptz,
    CONSTRAINT "idx_275912_primary" PRIMARY KEY ("id")
)
WITH (oids = false);

CREATE INDEX idx_275912_images_i5 ON photobase.photo_images USING btree (date_creation);

CREATE INDEX idx_275912_images_i9 ON photobase.photo_images USING btree (md5sum);

CREATE INDEX idx_275912_images_i8 ON photobase.photo_images USING btree (file);

CREATE INDEX idx_275912_images_i3 ON photobase.photo_images USING btree (path);

CREATE INDEX idx_275912_lastmodified ON photobase.photo_images USING btree (lastmodified);

CREATE INDEX idx_275912_images_i2 ON photobase.photo_images USING btree (date_available);

CREATE INDEX idx_275912_images_i6 ON photobase.photo_images USING btree (media_name);

CREATE INDEX idx_275912_images_i4 ON photobase.photo_images USING btree (filesize);


DELIMITER ;;

CREATE TRIGGER "on_update_current_timestamp" BEFORE UPDATE ON "photobase"."photo_images" FOR EACH ROW EXECUTE FUNCTION on_update_current_timestamp_photo_images();;

DELIMITER ;

DROP TABLE IF EXISTS "photo_thumbnails";
DROP SEQUENCE IF EXISTS photo_thumbnails_id_seq;
CREATE SEQUENCE photo_thumbnails_id_seq INCREMENT 1 MINVALUE 1 MAXVALUE 2147483647 CACHE 1;

CREATE TABLE "photobase"."photo_thumbnails" (
    "id" integer DEFAULT nextval('photo_thumbnails_id_seq') NOT NULL,
    "photo_id" integer NOT NULL,
    "size" integer NOT NULL,
    "width" integer,
    "height" integer,
    "format" character varying(10) DEFAULT 'jpeg',
    "image_data" bytea NOT NULL,
    "filesize" integer,
    "created_at" timestamptz DEFAULT now(),
    "md5sum" character(32),
    CONSTRAINT "photo_thumbnails_pkey" PRIMARY KEY ("id")
)
WITH (oids = false);

CREATE INDEX idx_photo_thumbnail_lookup ON photobase.photo_thumbnails USING btree (photo_id, size);


ALTER TABLE ONLY "photobase"."photo_thumbnails" ADD CONSTRAINT "photo_thumbnails_photo_id_fkey" FOREIGN KEY (photo_id) REFERENCES photo_images(id) ON DELETE CASCADE NOT DEFERRABLE;


