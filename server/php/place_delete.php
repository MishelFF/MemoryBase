<?php

require "config.php";

header("Content-Type: application/json");

$placeId = $_GET["id"] ?? null;

if (!is_numeric($placeId)) {
    http_response_code(400);
    echo json_encode(["error" => "id обязателен и должен быть числом"]);
    exit;
}

try {
    $stmt = $db->prepare("DELETE FROM photobase.places WHERE id = :id");
    $stmt->execute([":id" => (int)$placeId]);

    if ($stmt->rowCount() === 0) {
        http_response_code(404);
        echo json_encode(["error" => "Место не найдено"]);
        exit;
    }

    echo json_encode(["id" => (int)$placeId]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}