<?php

require "config.php";

header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
if (!is_array($input)) $input = [];

$mediaName = $input["media_name"] ?? "";
$folderPrefix = $input["folder_prefix"] ?? "";
$countryId = $input["country_id"] ?? null;

if ($mediaName === "" || !is_numeric($countryId)) {
    http_response_code(400);
    echo json_encode(["error" => "media_name и country_id обязательны"]);
    exit;
}

try {
    $stmt = $db->prepare("
        UPDATE photobase.photo_images
        SET country_id = :country_id
        WHERE media_name = :media_name
          AND (path = :prefix OR path LIKE :prefix_like)
    ");
    $stmt->execute([
        ":country_id" => (int)$countryId,
        ":media_name" => $mediaName,
        ":prefix" => $folderPrefix,
        ":prefix_like" => $folderPrefix . "/%",
    ]);

    echo json_encode(["updated" => $stmt->rowCount()]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}