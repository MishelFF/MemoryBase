<?php

require "config.php";

header("Content-Type: application/json");

$input = json_decode(file_get_contents("php://input"), true);
if (!is_array($input)) $input = [];

$name = trim($input["name"] ?? "");
$latitude = $input["latitude"] ?? null;
$longitude = $input["longitude"] ?? null;
$radiusKm = $input["radius_km"] ?? null;
$countryId = array_key_exists("country_id", $input) ? $input["country_id"] : null;

if ($name === "" || !is_numeric($latitude) || !is_numeric($longitude) || !is_numeric($radiusKm)) {
    http_response_code(400);
    echo json_encode(["error" => "name, latitude, longitude и radius_km обязательны"]);
    exit;
}

try {
    $stmt = $db->prepare("
        INSERT INTO photobase.places (name, latitude, longitude, radius_km, country_id)
        VALUES (:name, :latitude, :longitude, :radius_km, :country_id)
        RETURNING id
    ");
    $stmt->execute([
        ":name" => $name,
        ":latitude" => (float)$latitude,
        ":longitude" => (float)$longitude,
        ":radius_km" => (float)$radiusKm,
        ":country_id" => is_numeric($countryId) ? (int)$countryId : null,
    ]);
    $id = (int)$stmt->fetchColumn();

    echo json_encode([
        "id" => $id,
        "name" => $name,
        "latitude" => (float)$latitude,
        "longitude" => (float)$longitude,
        "radius_km" => (float)$radiusKm,
        "country_id" => is_numeric($countryId) ? (int)$countryId : null,
    ]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}