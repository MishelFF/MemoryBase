<?php

require "config.php";

header("Content-Type: application/json");

try {
    $stmt = $db->prepare("
        UPDATE photobase.photo_images
        SET country_id = (
            SELECT cb.country_id FROM photobase.country_bboxes cb
            WHERE photo_images.latitude  BETWEEN cb.lat_min AND cb.lat_max
              AND photo_images.longitude BETWEEN cb.lon_min AND cb.lon_max
            LIMIT 1
        )
        WHERE country_id IS NULL
          AND (latitude != 0 OR longitude != 0)
    ");
    $stmt->execute();

    echo json_encode(["updated" => $stmt->rowCount()]);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(["error" => $e->getMessage()]);
}