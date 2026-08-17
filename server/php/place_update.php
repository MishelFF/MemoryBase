<?php

require "config.php";

header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
if (!is_array($input)) $input = [];

$id = $input["id"] ?? null;
$name = trim($input["name"] ?? "");
$radiusKm = $input["radius_km"] ?? null;
$countryId = array_key_exists("country_id", $input) ? $input["country_id"] : null;

if (!is_numeric($id) || $name === "" || !is_numeric($radiusKm)) {
    http_response_code(400);
    echo json_encode(["error" => "id, name и radius_km обязательны"]);
    exit;
}

try {
    $stmt = $db->prepare("
        UPDATE photobase.places
        SET name = :name, radius_km = :radius_km, country_id = :country_id
        WHERE id = :id
        RETURNING latitude, longitude
    ");
    $stmt->execute([
        ":name" => $name,
        ":radius_km" => (float)$radiusKm,
        ":country_id" => is_numeric($countryId) ? (int)$countryId : null,
        ":id" => (int)$id,
    ]);
    $row = $stmt->fetch(PDO::FETCH_ASSOC);

    if ($row === false) {
        http_response_code(404);
        echo json_encode(["error" => "Место не найдено"]);
        exit;
    }

    echo json_encode([
        "id" => (int)$id,
        "name" => $name,
        "latitude" => (float)$row["latitude"],
        "longitude" => (float)$row["longitude"],
        "radius_km" => (float)$radiusKm,
        "country_id" => is_numeric($countryId) ? (int)$countryId : null,
    ]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}