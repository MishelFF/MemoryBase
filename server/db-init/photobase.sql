CREATE SCHEMA IF NOT EXISTS "photobase";

DROP FUNCTION IF EXISTS "photobase"."on_update_current_timestamp_photo_images";
CREATE FUNCTION "photobase"."on_update_current_timestamp_photo_images" () RETURNS trigger LANGUAGE plpgsql AS '
BEGIN
   NEW.lastmodified = now();
   RETURN NEW;
END;
';

DROP TABLE IF EXISTS "photobase"."media_mounts";
CREATE TABLE "photobase"."media_mounts" (
    "media_name" text NOT NULL,
    "mount_point" text NOT NULL,
    CONSTRAINT "media_mounts_pkey" PRIMARY KEY ("media_name")
)
WITH (oids = false);

DROP TABLE IF EXISTS "photobase"."photo_images";
DROP SEQUENCE IF EXISTS photobase.photo_images_id_seq;
CREATE SEQUENCE photobase.photo_images_id_seq INCREMENT 1 MINVALUE 1 MAXVALUE 2147483647 CACHE 1;

CREATE TABLE "photobase"."photo_images" (
    "id" integer DEFAULT nextval('photobase.photo_images_id_seq') NOT NULL,
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
    "missing_since" timestamptz,
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

CREATE TRIGGER "on_update_current_timestamp" BEFORE UPDATE ON "photobase"."photo_images"
    FOR EACH ROW EXECUTE FUNCTION photobase.on_update_current_timestamp_photo_images();

DROP TABLE IF EXISTS "photobase"."photo_thumbnails";
DROP SEQUENCE IF EXISTS photobase.photo_thumbnails_id_seq;
CREATE SEQUENCE photobase.photo_thumbnails_id_seq INCREMENT 1 MINVALUE 1 MAXVALUE 2147483647 CACHE 1;

CREATE TABLE "photobase"."photo_thumbnails" (
    "id" integer DEFAULT nextval('photobase.photo_thumbnails_id_seq') NOT NULL,
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

ALTER TABLE ONLY "photobase"."photo_images"
    ADD CONSTRAINT "fk_photo_images_media" FOREIGN KEY (media_name)
    REFERENCES photobase.media_mounts(media_name) ON UPDATE CASCADE NOT DEFERRABLE;

ALTER TABLE ONLY "photobase"."photo_thumbnails"
    ADD CONSTRAINT "photo_thumbnails_photo_id_fkey" FOREIGN KEY (photo_id)
    REFERENCES photobase.photo_images(id) ON DELETE CASCADE NOT DEFERRABLE;
CREATE TABLE "photobase"."licenses"
(
    id                  BIGSERIAL PRIMARY KEY,
    license_key         VARCHAR(32)  NOT NULL UNIQUE,
    client              VARCHAR(200) NOT NULL,
    email               VARCHAR(200),
    edition             VARCHAR(50)  NOT NULL DEFAULT 'Standard',
    expires             DATE,
    enabled             BOOLEAN      NOT NULL DEFAULT TRUE,
    max_activations     INTEGER      NOT NULL DEFAULT 1,
    created_at          TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE "photobase"."license_activations"
(
    id                  BIGSERIAL PRIMARY KEY,
    license_id          BIGINT NOT NULL REFERENCES licenses(id) ON DELETE CASCADE,
    machine_id          VARCHAR(64) NOT NULL,
    activated_at        TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_seen           TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (license_id, machine_id)
);

CREATE INDEX idx_license_key
    ON photobase.licenses(license_key);

CREATE INDEX idx_activation_machine
    ON photobase.license_activations(machine_id);