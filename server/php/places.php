<?php

require "config.php";

header("Content-Type: application/json");

try {
    $stmt = $db->query("
        SELECT id, name, latitude, longitude, radius_km, country_id
        FROM photobase.places
        ORDER BY name
    ");
    $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);

    $result = array_map(function ($p) {
        return [
            "id" => (int)$p["id"],
            "name" => $p["name"],
            "latitude" => (float)$p["latitude"],
            "longitude" => (float)$p["longitude"],
            "radius_km" => (float)$p["radius_km"],
            "country_id" => $p["country_id"] === null ? null : (int)$p["country_id"],
        ];
    }, $rows);

    echo json_encode($result);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}