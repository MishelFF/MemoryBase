<?php
declare(strict_types=1);

class LicenseSigner
{
    private string $secretKey;

    public function __construct(string $keyFile)
    {
        $this->secretKey = file_get_contents($keyFile);

        if (strlen($this->secretKey) !== SODIUM_CRYPTO_SIGN_SECRETKEYBYTES)
            throw new Exception("Invalid secret key.");
    }

    public function sign(array $payload): array
    {
        ksort($payload);

        $json = json_encode(
            $payload,
            JSON_UNESCAPED_UNICODE |
            JSON_UNESCAPED_SLASHES
        );

        $signature = sodium_crypto_sign_detached(
            $json,
            $this->secretKey
        );

        return [
            "payload" => $payload,
            "signature" => base64_encode($signature)
        ];
    }
}