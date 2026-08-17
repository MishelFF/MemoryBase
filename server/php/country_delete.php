<?php

require "config.php";

header("Content-Type: application/json");

$countryId = $_GET["id"] ?? null;

if (!is_numeric($countryId)) {
    http_response_code(400);
    echo json_encode(["error" => "id обязателен и должен быть числом"]);
    exit;
}

try {
    $stmt = $db->prepare("DELETE FROM photobase.countries WHERE id = :id");
    $stmt->execute([":id" => (int)$countryId]);

    if ($stmt->rowCount() === 0) {
        http_response_code(404);
        echo json_encode(["error" => "Страна не найдена"]);
        exit;
    }

    echo json_encode(["id" => (int)$countryId]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}