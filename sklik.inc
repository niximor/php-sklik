<?php

/**
 * Error thrown when communicating with Sklik API
 */
class SklikError extends Exception {
    function __construct($message, $code, $method, $args) {
        parent::__construct("While calling ".$method."(".var_export($args, true).") server returned: ".$message, $code);
        $this->method = $method;
        $this->args = $args;
    }

    function getMethod() {
        return $this->method;
    }

    function getArgs() {
        return $this->args;
    }
}

/**
 * Exception thrown when method call does not return status=2xx.
 */
class MethodError extends SklikError {
    protected $response;

    function __construct($message, $code, $response, $method, $args) {
        parent::__construct($message, $code, $method, $args);
        $this->response = $response;
    }

    function getResponse() {
        return $this->response;
    }
}

/**
 * Support class to call Sklik.
 */
class SklikCall {
    protected $client; // XML-RPC client to Sklik
    protected $method; // Method name

    function __construct(Sklik $client, $method) {
        $this->client = $client;
        $this->method = $method;
    }

    function __get($key) {
        return new SklikCall($this->client, $this->method.".".$key);
    }

    function __call($method, $args) {
        return call_user_func($this->client, $this->method.".".$method, $args);
    }

    function __invoke(/* args */) {
        $args = func_get_args();
        return call_user_func($this->client, $this->method, $args);
    }
}

/**
 * Sklik API library
 */
class Sklik {
    // XML-RPC client
    protected $client;

    // List of methods, that does not accept session parameter.
    protected $noSession = array(
        "api.version", "client.login"
    );

    protected $session = NULL; // Login session, acquired automatically from API
    protected $userId = NULL; // Serviced user ID, NULL to use logged-in account identified by session.

    function __construct($username, $password, $sandbox=true) {
        $GLOBALS["xmlrpc_null_extension"] = true; // We need to work with NULL
        $GLOBALS["xmlrpc_internalencoding"] = "UTF-8"; // We work with UTF-8

        // RPC connection
        $this->client = new xmlrpc_client(($sandbox)?'/sandbox/cipisek/RPC2':'/cipisek/RPC2', 'api.sklik.cz', 443, 'https');
        $this->client->setSSLVerifyHost(2);

        // Login user
        $this->login($username, $password);
    }

    /**
     * Login user
     */
    function login($username, $password) {
        $res = $this->__get("client")->login($username, $password);
    }

    /**
     * Returns true, if method needs session parameter.
     */
    function needSession($method) {
        return !in_array($method, $this->noSession);
    }

    /**
     * Returns prepared session parameter for method calls.
     */
    function getSession() {
        $res = array(
            "session" => $this->session
        );
        if (!is_null($this->userId)) {
            $res["userId"] = $this->userId;
        }

        return $res;
    }

    /**
     * Sets userId to manage foreign accounts.
     */
    function setUserId($userId) {
        $this->userId = $userId;
    }

    /**
     * Method call helper
     */
    function __get($method) {
        return new SklikCall($this, $method);
    }

    /**
     * Method call helper
     */
    function __invoke($method, $args) {
        if ($this->needSession($method)) {
            $args = array_merge(
                array($this->getSession()),
                $args
            );
        }

        $encoded = array_map("php_xmlrpc_encode", $args);

        $msg = new xmlrpcmsg($method, $encoded);
        $resp = $this->client->send($msg);
        if (!$resp->faultCode()) {
            $res = php_xmlrpc_decode($resp->value());

            // Refresh session
            if (isset($res["session"])) {
                $this->session = $res["session"];
            }

            if ($res["status"] / 100 != 2) {
                throw new MethodError($res["statusMessage"], $res["status"], $res, $method, $args);
            }

            return $res;
        } else {
            throw new SklikError($resp->faultString(), $resp->faultCode(), $method, $args);
        }

    }

    /**
     * Create DateTime object from unix timestamp.
     */
    static function dateTime($time) {
        $dt = new DateTime();
        $dt->setTimestamp($time);
        return $dt;
    }
}

