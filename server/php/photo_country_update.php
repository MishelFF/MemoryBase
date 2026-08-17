<?php

require "config.php";

header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
if (!is_array($input)) $input = [];

$photoId = $input["photo_id"] ?? null;
$countryId = array_key_exists("country_id", $input) ? $input["country_id"] : null;

if (!is_numeric($photoId)) {
    http_response_code(400);
    echo json_encode(["error" => "photo_id обязателен и должен быть числом"]);
    exit;
}

try {
    $stmt = $db->prepare("
        UPDATE photobase.photo_images
        SET country_id = :country_id
        WHERE id = :photo_id
    ");
    $stmt->execute([
        ":country_id" => is_numeric($countryId) ? (int)$countryId : null,
        ":photo_id" => (int)$photoId,
    ]);

    if ($stmt->rowCount() === 0) {
        http_response_code(404);
        echo json_encode(["error" => "Фото не найдено"]);
        exit;
    }

    echo json_encode(["photo_id" => (int)$photoId, "country_id" => is_numeric($countryId) ? (int)$countryId : null]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}