<?php
declare(strict_types=1);

header('Content-Type: application/json');

require_once "config.php";
require_once "LicenseSigner.php";

$request = json_decode(file_get_contents("php://input"), true);

if (!$request) {
    http_response_code(400);
    exit(json_encode(["status" => "error", "message" => "Invalid JSON"]));
}

$licenseKey = trim($request["license"] ?? "");
$machineId  = trim($request["machine"] ?? "");

if ($licenseKey === "") {
    exit(json_encode(["status" => "error", "message" => "License key required"]));
}

$sql = "SELECT * FROM photobase.licenses WHERE license_key = :key AND enabled = TRUE";

$stmt = $db->prepare($sql);
$stmt->execute(["key" => $licenseKey]);

$row = $stmt->fetch(PDO::FETCH_ASSOC);

if (!$row) {
    exit(json_encode([
        "status" => "error",
        "message" => "License not found"
    ]));
}

if ($row["expires"] !== null &&
    strtotime($row["expires"]) < time())
{
    exit(json_encode([
        "status" => "error",
        "message" => "License expired"
    ]));
}

/*
 * Здесь позже будет проверка machine_id
 * и количества активаций.
 */

$payload = [
    "customer" => $row["customer"],
    "edition"  => $row["edition"],
    "expires"  => $row["expires"],
    "machine"  => $machineId
];

$signer = new LicenseSigner(__DIR__ . "/private.key");

$license = $signer->sign($payload);

echo json_encode([
    "status"  => "ok",
    "license" => $license
]);