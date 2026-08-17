<?php

require "config.php";

header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
if (!is_array($input)) $input = [];

$countryId = $input["country_id"] ?? null;
$bboxes = isset($input["bboxes"]) && is_array($input["bboxes"]) ? $input["bboxes"] : [];

if (!is_numeric($countryId)) {
    http_response_code(400);
    echo json_encode(["error" => "country_id обязателен и должен быть числом"]);
    exit;
}

try {
    $db->beginTransaction();

    $del = $db->prepare("DELETE FROM photobase.country_bboxes WHERE country_id = :country_id");
    $del->execute([":country_id" => (int)$countryId]);

    $ins = $db->prepare("
        INSERT INTO photobase.country_bboxes (country_id, lat_min, lat_max, lon_min, lon_max)
        VALUES (:country_id, :lat_min, :lat_max, :lon_min, :lon_max)
    ");

    foreach ($bboxes as $b) {
        $ins->execute([
            ":country_id" => (int)$countryId,
            ":lat_min" => (float)($b["lat_min"] ?? 0),
            ":lat_max" => (float)($b["lat_max"] ?? 0),
            ":lon_min" => (float)($b["lon_min"] ?? 0),
            ":lon_max" => (float)($b["lon_max"] ?? 0),
        ]);
    }

    $db->commit();

    echo json_encode(["country_id" => (int)$countryId, "count" => count($bboxes)]);

} catch (PDOException $e) {
    $db->rollBack();
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}