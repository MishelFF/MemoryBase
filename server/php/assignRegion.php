<?php

require "config.php";
header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
$regionId = $input["region_id"] ?? null;
$personId = $input["person_id"] ?? null;

if (!is_numeric($regionId) || !is_numeric($personId)) {
    http_response_code(400);
    echo json_encode(["error" => "region_id и person_id обязательны и должны быть числами"]);
    exit;
}

try {
    $stmt = $db->prepare("
        UPDATE photobase.photo_regions SET person_id = :person_id WHERE id = :region_id");
    $stmt->execute([":person_id" => (int)$personId,":region_id" => (int)$regionId,]);
    echo json_encode(["region_id" => (int)$regionId,"person_id" => (int)$personId,]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}