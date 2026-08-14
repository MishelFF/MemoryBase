<?php

require "config.php";
header("Content-Type: application/json");

$stmt = $db->query("
    SELECT id, display_name, reference_chip IS NOT NULL AS has_reference
    FROM photobase.people ORDER BY display_name
");

echo json_encode($stmt->fetchAll(PDO::FETCH_ASSOC));