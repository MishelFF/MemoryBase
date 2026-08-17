<?php

require "config.php";

header("Content-Type: application/json");

try {
    $countries = $db->query("
        SELECT id, name FROM photobase.countries ORDER BY name")->fetchAll(PDO::FETCH_ASSOC);

    $bboxStmt = $db->prepare("SELECT id, lat_min, lat_max, lon_min, lon_max
        FROM photobase.country_bboxes WHERE country_id = :country_id");

    $result = [];
    foreach ($countries as $c) {
        $bboxStmt->execute([":country_id" => (int)$c["id"]]);
        $bboxes = $bboxStmt->fetchAll(PDO::FETCH_ASSOC);

        $result[] = ["id" => (int)$c["id"],"name" => $c["name"],
            "bboxes" => array_map(function ($b) {
                return [
                    "id" => (int)$b["id"],
                    "lat_min" => (float)$b["lat_min"],
                    "lat_max" => (float)$b["lat_max"],
                    "lon_min" => (float)$b["lon_min"],
                    "lon_max" => (float)$b["lon_max"],
                ];
            }, $bboxes),
        ];
    }

    echo json_encode($result);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}