<?php

require "config.php";
header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
$personId = $input["person_id"] ?? null;
$regionId = $input["region_id"] ?? null;

if (!is_numeric($personId) || !is_numeric($regionId)) {
    http_response_code(400);
    echo json_encode(["error" => "person_id и region_id обязательны и должны быть числами"]);
    exit;
}

try {
    $stmt = $db->prepare("
        UPDATE photobase.people
        SET reference_chip = src.face_chip,reference_descriptor = src.descriptor,
            reference_descriptor_model = src.descriptor_model,reference_source_region_id = src.id
        FROM photobase.photo_regions src WHERE photobase.people.id = :person_id AND src.id = :region_id
    ");
    $stmt->execute([":person_id" => (int)$personId,":region_id" => (int)$regionId,]);

    if ($stmt->rowCount() === 0) {
        http_response_code(404);
        echo json_encode(["error" => "Не найдена пара person_id/region_id"]);
        exit;
    }

    echo json_encode(["person_id" => (int)$personId,"region_id" => (int)$regionId,]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}