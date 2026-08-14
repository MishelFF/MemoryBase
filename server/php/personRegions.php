<?php

require "config.php";
header("Content-Type: application/json");

$personId = $_GET["person_id"] ?? null;

if (!ctype_digit((string)$personId)) {
    http_response_code(400);
    echo json_encode(["error" => "Некорректный person_id"]);
    exit;
}

$stmt = $db->prepare("
    SELECT id, photo_id, face_name FROM photobase.photo_regions
    WHERE person_id = :person_id ORDER BY photo_id, id
");
$stmt->execute([":person_id" => (int)$personId]);

echo json_encode(["person_id" => (int)$personId,"regions"   => $stmt->fetchAll(PDO::FETCH_ASSOC),]);