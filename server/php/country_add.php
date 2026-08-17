<?php

require "config.php";

header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
if (!is_array($input)) $input = [];

$name = trim($input["name"] ?? "");
$bboxes = isset($input["bboxes"]) && is_array($input["bboxes"]) ? $input["bboxes"] : [];

if ($name === "") {
    http_response_code(400);
    echo json_encode(["error" => "name обязателен"]);
    exit;
}

try {
    $db->beginTransaction();

    $stmt = $db->prepare("
        INSERT INTO photobase.countries (name) VALUES (:name) RETURNING id
    ");
    $stmt->execute([":name" => $name]);
    $countryId = (int)$stmt->fetchColumn();

    $bboxStmt = $db->prepare("
        INSERT INTO photobase.country_bboxes (country_id, lat_min, lat_max, lon_min, lon_max)
        VALUES (:country_id, :lat_min, :lat_max, :lon_min, :lon_max)
    ");

    foreach ($bboxes as $b) {
        $bboxStmt->execute([
            ":country_id" => $countryId,
            ":lat_min" => (float)($b["lat_min"] ?? 0),
            ":lat_max" => (float)($b["lat_max"] ?? 0),
            ":lon_min" => (float)($b["lon_min"] ?? 0),
            ":lon_max" => (float)($b["lon_max"] ?? 0),
        ]);
    }

    $db->commit();

    echo json_encode(["id" => $countryId, "name" => $name]);

} catch (PDOException $e) {
    $db->rollBack();
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}