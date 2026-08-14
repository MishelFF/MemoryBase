<?php

require "config.php";
header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
$regionId = $input["region_id"] ?? null;

if (!is_numeric($regionId)) {
    http_response_code(400);
    echo json_encode(["error" => "region_id обязателен и должен быть числом"]);
    exit;
}

try {
    $stmt = $db->prepare("UPDATE photobase.photo_regions SET person_id = NULL WHERE id = :region_id");
    $stmt->execute([":region_id" => (int)$regionId]);

    echo json_encode(["region_id" => (int)$regionId]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}