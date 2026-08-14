<?php

require "config.php";
header("Content-Type: application/json");

$stmt = $db->query("
    SELECT id, photo_id, face_name FROM photobase.photo_regions
    WHERE person_id IS NULL ORDER BY photo_id, id
");

echo json_encode($stmt->fetchAll(PDO::FETCH_ASSOC));