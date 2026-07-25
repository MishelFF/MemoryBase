<?php

require "config.php";

header("Content-Type: application/json");

$id = $_GET["id"];

$stmt = $db->prepare("SELECT id, file, width, height, date_creation, comment, device, maker, filesize, ext, path, media_name, md5sum, rotation, latitude, longitude, lastmodified FROM photobase.photo_images WHERE id=:id");

$stmt->execute([":id" => $id]);

$row = $stmt->fetch(PDO::FETCH_ASSOC);

echo json_encode($row ?: []);