# TMNN.js: Cloud-Native Serverless Usenet for the Modern Web

**Move fast and post things.**

## The Vision

It's 2026. Why are we still thinking about Usenet as a *desktop application*?

TMNN.js reimagines news as a **microservices architecture**:
- Article Service (Lambda)
- Group Service (Lambda)  
- Auth Service (Cognito)
- Search Service (Elasticsearch)
- Cache Layer (Redis)
- Message Queue (SQS)
- API Gateway (duh)
- Frontend (React + Next.js + Tailwind)

All serverless. All auto-scaling. All *webscale*.

## Why Node?

- **JavaScript is the language of the web**
- **npm has a package for everything**
- **async/await just works**
- **JSON is native** (not that XML garbage)
- **The community is massive**

The original C code had to manage its own memory like a caveman. We have garbage collection. We have npm. We have *the cloud*.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     CloudFront CDN                          │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                      API Gateway                            │
└─────────────────────────────────────────────────────────────┘
          │              │              │              │
     ┌────┴────┐    ┌────┴────┐    ┌────┴────┐    ┌────┴────┐
     │ Article │    │  Group  │    │  Auth   │    │ Search  │
     │ Lambda  │    │ Lambda  │    │ Lambda  │    │ Lambda  │
     └────┬────┘    └────┬────┘    └────┬────┘    └────┬────┘
          │              │              │              │
     ┌────┴────────────────────────────────────────────┴────┐
     │                      DynamoDB                        │
     └──────────────────────────────────────────────────────┘
```

## Current Status

🚀 **Moving Fast** 🚀

- [x] Created package.json
- [x] Set up Serverless Framework
- [ ] Implement article CRUD
- [ ] Add GraphQL layer (REST is so 2015)
- [ ] Deploy to AWS
- [ ] Add Kubernetes option for on-prem (enterprise customers)

## For the Haters

"But Usenet doesn't need microservices!"

Counterpoint: Have you SEEN the original code? It doesn't even have an API! How are you supposed to integrate with Slack? Or Discord? Or your enterprise workflow?

The future is API-first. The future is cloud-native. The future is **webscale**.

---

*WebScaleChad*

🚀 Ship it! 🚀
